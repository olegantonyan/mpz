#include "statusbarlabel.h"

#include <QDebug>
#include <QRegularExpression>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>

StatusBarLabel::StatusBarLabel(QWidget *parent) : QLabel(parent) {
  on_playerStopped();
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QLabel::customContextMenuRequested, this, &StatusBarLabel::on_contextMenu);
}

void StatusBarLabel::on_playerStopped() {
  _state = tr("Stopped");
  _stream_buffer = 0;
  setText(_state);
}

void StatusBarLabel::on_playerStarted(const Track &track) {
  _state = tr("Playing");
  auto t = _state + trackInfo(track);
  _stream_buffer = 0;
  setText(t);
}

void StatusBarLabel::on_playerPaused(const Track &track) {
  _state = tr("Paused");
  auto t = _state + trackInfo(track);
  setText(t);
}

void StatusBarLabel::on_streamBufferFill(const Track &track, quint32 bytes) {
  _stream_buffer = bytes;
  if (_state != tr("Stopped")) {
    setText(_state + trackInfo(track));
  }
}

void StatusBarLabel::on_progress(const Track &track, int current_seconds) {
  Q_UNUSED(current_seconds)
  setText(_state + trackInfo(track));
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  emit doubleclicked();
  QLabel::mouseDoubleClickEvent(event);
}

void StatusBarLabel::on_contextMenu(const QPoint &pos) {
  QMenu menu;
  QAction copy_name(tr("Copy"));
  connect(&copy_name, &QAction::triggered, [=]() {
     qApp->clipboard()->setText(trackTitle());
  });
  QAction show_log(tr("Show playback log"));
  connect(&show_log, &QAction::triggered, this, &StatusBarLabel::showPlaybackLog);
  QAction jump_to(tr("Jump to playing track"));
  connect(&jump_to, &QAction::triggered, this, &StatusBarLabel::doubleclicked);
  QAction search(tr("Search on web"));
  connect(&search, &QAction::triggered, [=]() {
    auto term = QString("https://duckduckgo.com/?q=%1").arg(QString(QUrl::toPercentEncoding(trackTitle())));
    QDesktopServices::openUrl(QUrl(term));
  });

  menu.addAction(&jump_to);
  menu.addSeparator();
  menu.addAction(&copy_name);
  menu.addAction(&search);
  menu.addAction(&show_log);
  menu.exec(mapToGlobal(pos));
}

QString StatusBarLabel::trackInfo(const Track &t) const {
  auto c = ": " + t.shortText() + " | " + t.formattedAudioInfo();
  if (_stream_buffer > 0) {
    c += QString(" | %1 %2").arg(tr("stream buffer")).arg(humanized_bytes(_stream_buffer));
  }
  return c;
}

QString StatusBarLabel::humanized_bytes(quint32 bytes) const {
    QStringList list;
    list << tr("KB") << tr("MB") << tr("GB") << tr("TB");

    QStringListIterator i(list);
    QString unit(tr("bytes"));

    while(bytes >= 1024 && i.hasNext()) {
      unit = i.next();
      bytes /= 1024;
    }
    return QString().setNum(bytes, 10) + " " + unit;
}

QString StatusBarLabel::trackTitle() const {
  return text().split("|").first().replace(_state + ": ", "");
}
