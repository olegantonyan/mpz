#include "dynamic_range_ui/dynamicrangedialog.h"
#include "ui_dynamicrangedialog.h"

#include "dynamic_range/logformat.h"
#include "icons.h"

#include <QApplication>
#include <QAudioDecoder>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSet>
#include <QStandardPaths>

#include <algorithm>

namespace {
  QString displayName(const Track &t) {
    const QString title = t.title().isEmpty() ? t.filename() : t.title();
    if (t.track_number() > 0) {
      return QString("%1-%2").arg(t.track_number(), 2, 10, QChar('0')).arg(title);
    }
    return title;
  }

  QVector<DynamicRange::Job> buildJobs(const QVector<Track> &tracks) {
    QVector<DynamicRange::Job> jobs;
    QHash<QString, int> job_of_path;
    QSet<QString> seen;

    for (const auto &t : tracks) {
      const QString key = QString("%1|%2|%3").arg(t.path()).arg(t.begin()).arg(t.duration());
      if (seen.contains(key)) {
        continue;
      }
      seen.insert(key);

      DynamicRange::Segment s;
      s.uid = t.uid();
      s.begin_us = qint64(t.begin()) * 1000;
      s.end_us = t.isCue() && t.duration() > 0 ? s.begin_us + qint64(t.duration()) * 1000 : -1;
      s.duration_us = qint64(t.duration()) * 1000;

      if (!job_of_path.contains(t.path())) {
        job_of_path.insert(t.path(), jobs.size());
        DynamicRange::Job job;
        job.path = t.path();
        jobs.append(job);
      }
      jobs[job_of_path.value(t.path())].segments.append(s);
    }

    for (auto &job : jobs) {
      std::sort(job.segments.begin(), job.segments.end(),
                [](const DynamicRange::Segment &a, const DynamicRange::Segment &b) {
                  return a.begin_us < b.begin_us;
                });
    }
    return jobs;
  }
}

DynamicRangeDialog::DynamicRangeDialog(const QVector<Track> &selection, QWidget *parent)
    : QDialog(parent), ui(new Ui::DynamicRangeDialog), tracks(selection) {
  ui->setupUi(this);
  ui->textLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  ui->buttonCopy->setIcon(Icons::get(Icons::Icon::Copy));
  ui->buttonSaveAs->setIcon(Icons::get(Icons::Icon::Save));
  ui->buttonCopy->setEnabled(false);
  ui->buttonSaveAs->setEnabled(false);
  ui->progressBar->setMaximum(0);

  QAudioDecoder probe;
  if (!probe.isSupported()) {
    ui->labelStatus->setText(tr("Audio decoding is not available"));
    ui->progressBar->hide();
    return;
  }

  scanner = new DynamicRange::Scanner(this);
  connect(scanner, &DynamicRange::Scanner::progress, this, &DynamicRangeDialog::on_progress);
  connect(scanner, &DynamicRange::Scanner::segmentDone, this, &DynamicRangeDialog::on_segmentDone);
  connect(scanner, &DynamicRange::Scanner::finished, this, &DynamicRangeDialog::on_finished);

  running = true;
  ui->buttonClose->setText(tr("Cancel"));
  updateStatus();
  scanner->run(buildJobs(tracks));
}

DynamicRangeDialog::~DynamicRangeDialog() {
  delete ui;
}

void DynamicRangeDialog::reject() {
  if (running) {
    running = false;
    scanner->cancel();
  }
  QDialog::reject();
}

void DynamicRangeDialog::on_buttonClose_clicked() {
  reject();
}

void DynamicRangeDialog::on_buttonCopy_clicked() {
  qApp->clipboard()->setText(ui->textLog->toPlainText());
}

void DynamicRangeDialog::on_buttonSaveAs_clicked() {
  const QString suggested = QString("%1/%2.txt")
                                .arg(tracks.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
                                                      : tracks.first().dir(),
                                     tracks.isEmpty() || tracks.first().album().isEmpty()
                                         ? QStringLiteral("dr")
                                         : tracks.first().album());
  const QString filename = QFileDialog::getSaveFileName(this, tr("Save dynamic range log"), suggested,
                                                        tr("Text files (*.txt)"));
  if (filename.isEmpty()) {
    return;
  }
  QFile f(filename);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, tr("Save failed"), f.errorString());
    return;
  }
  f.write(ui->textLog->toPlainText().toUtf8());
  f.close();
}

void DynamicRangeDialog::on_progress(qint64 done_us, qint64 total_us) {
  ui->progressBar->setMaximum(int(total_us / 1000));
  ui->progressBar->setValue(int(qBound(qint64(0), done_us, total_us) / 1000));
}

void DynamicRangeDialog::on_segmentDone(quint64 uid, const DynamicRange::Result &result) {
  results.insert(uid, result);
  ++done_count;
  updateStatus();
}

void DynamicRangeDialog::on_finished(bool cancelled) {
  if (cancelled) {
    return;
  }
  running = false;
  ui->labelStatus->hide();
  ui->progressBar->hide();
  ui->textLog->setPlainText(buildLog());
  ui->buttonCopy->setEnabled(true);
  ui->buttonSaveAs->setEnabled(true);
  ui->buttonClose->setText(tr("Close"));
}

void DynamicRangeDialog::updateStatus() {
  if (tracks.isEmpty()) {
    return;
  }
  const int index = qMin(done_count, tracks.size() - 1);
  ui->labelStatus->setText(tr("Analyzing %1/%2: %3")
                               .arg(index + 1)
                               .arg(tracks.size())
                               .arg(tracks.at(index).formattedTitle()));
}

QString DynamicRangeDialog::buildLog() const {
  QVector<DynamicRange::Entry> entries;
  entries.reserve(tracks.size());
  for (const auto &t : tracks) {
    const Track::AudioProperties props = Track::audioPropertiesOf(t.path());
    DynamicRange::Entry e;
    e.artist = t.album_artist().isEmpty() ? t.artist() : t.album_artist();
    e.album = t.album();
    e.display = displayName(t);
    e.duration_ms = t.duration();
    e.sample_rate = t.sample_rate() > 0 ? t.sample_rate() : props.sample_rate;
    e.channels = t.channels() > 0 ? t.channels() : props.channels;
    e.bits_per_sample = props.bits_per_sample;
    e.bitrate = t.bitrate() > 0 ? t.bitrate() : props.bitrate;
    e.codec = t.format();
    e.result = results.value(t.uid());
    entries << e;
  }

  DynamicRange::LogMeta meta;
  meta.app_version = qApp->applicationVersion();
  meta.when = QDateTime::currentDateTime();
  return DynamicRange::formatLog(entries, meta);
}
