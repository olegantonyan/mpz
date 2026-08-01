#include <QtTest>
#include <QDataStream>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <cmath>
#include <functional>

#include "replaygain/scanner.h"
#include "replaygain/tags.h"

namespace {
  const int kRate = 44100;
  const int kChannels = 2;
  const int kScanTimeoutMs = 30000;

  bool writeWav(const QString &path, int frames, const std::function<double(int)> &amplitude) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
      return false;
    }
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    const quint32 data_bytes = static_cast<quint32>(frames) * kChannels * 2;
    f.write("RIFF");
    out << quint32(36 + data_bytes);
    f.write("WAVE");
    f.write("fmt ");
    out << quint32(16) << quint16(1) << quint16(kChannels) << quint32(kRate)
        << quint32(kRate * kChannels * 2) << quint16(kChannels * 2) << quint16(16);
    f.write("data");
    out << data_bytes;

    for (int i = 0; i < frames; i++) {
      const double v = amplitude(i) * std::sin(2.0 * M_PI * 997.0 * i / kRate);
      const qint16 s = static_cast<qint16>(std::lround(std::clamp(v, -1.0, 1.0) * 32767.0));
      out << s << s;
    }
    f.close();
    return true;
  }

  ReplayGain::Job jobFor(const QString &folder, const QVector<ReplayGain::FileWork> &files,
                         bool want_album) {
    ReplayGain::Job job;
    job.folder = folder;
    job.files = files;
    job.want_album = want_album;
    return job;
  }

  ReplayGain::FileWork wholeFile(const QString &path) {
    ReplayGain::FileWork w;
    w.path = path;
    w.slices.append(ReplayGain::Slice{0, 0});
    return w;
  }
}

class TestReplayGainScanner : public QObject {
  Q_OBJECT
private slots:
  void init();

  void scansASingleFile();
  void silenceIsReportedAsFailure();
  void missingFileIsReportedAsFailure();
  void trackModeLeavesAlbumEmpty();
  void albumGainIsSharedAcrossTheFolder();
  void cueSlicesAreMeasuredSeparately();
  void multiSliceFileCannotWriteTags();
  void emptyRequestFinishesImmediately();
  void progressReachesTotal();
  void cancelStopsTheScan();

private:
  QString sine(const QString &name, double amplitude, int seconds = 2);
  QVector<ReplayGain::SliceResult> runScan(const QVector<ReplayGain::Job> &jobs, int workers = 1);

  std::unique_ptr<QTemporaryDir> dir;
  int last_analysed = 0;
  int last_failed = 0;
  bool last_cancelled = false;
};

void TestReplayGainScanner::init() {
  dir = std::make_unique<QTemporaryDir>();
  dir->setAutoRemove(true);
  QVERIFY(dir->isValid());
  last_analysed = 0;
  last_failed = 0;
  last_cancelled = false;
}

QString TestReplayGainScanner::sine(const QString &name, double amplitude, int seconds) {
  const QString path = dir->filePath(name);
  if (!writeWav(path, kRate * seconds, [amplitude](int) { return amplitude; })) {
    return QString();
  }
  return path;
}

QVector<ReplayGain::SliceResult> TestReplayGainScanner::runScan(
    const QVector<ReplayGain::Job> &jobs, int workers) {
  ReplayGain::Scanner scanner;
  QVector<ReplayGain::SliceResult> results;

  connect(&scanner, &ReplayGain::Scanner::sliceAnalyzed, this,
          [&results](const ReplayGain::SliceResult &r) { results.append(r); });
  connect(&scanner, &ReplayGain::Scanner::finished, this,
          [this](int analysed, int failed, bool cancelled) {
            last_analysed = analysed;
            last_failed = failed;
            last_cancelled = cancelled;
          });

  QSignalSpy done(&scanner, &ReplayGain::Scanner::finished);
  scanner.start(jobs, workers);
  if (!done.isEmpty() || done.wait(kScanTimeoutMs)) {
    return results;
  }
  return results;
}

void TestReplayGainScanner::scansASingleFile() {
  const QString path = sine(QStringLiteral("tone.wav"), 0.5);
  QVERIFY(!path.isEmpty());

  const auto results = runScan({jobFor(dir->path(), {wholeFile(path)}, false)});
  QCOMPARE(results.size(), 1);
  QVERIFY2(results.at(0).ok, qPrintable(results.at(0).error));
  QCOMPARE(results.at(0).path, path);
  QVERIFY(results.at(0).gain.has_track);
  QVERIFY(std::isfinite(results.at(0).gain.track_db));
  QVERIFY(results.at(0).gain.track_peak > 0.4);
  QCOMPARE(last_analysed, 1);
  QCOMPARE(last_failed, 0);
  QVERIFY(!last_cancelled);
}

void TestReplayGainScanner::silenceIsReportedAsFailure() {
  const QString path = sine(QStringLiteral("silence.wav"), 0.0);

  const auto results = runScan({jobFor(dir->path(), {wholeFile(path)}, false)});
  QCOMPARE(results.size(), 1);
  QVERIFY(!results.at(0).ok);
  QVERIFY(!results.at(0).error.isEmpty());
  QCOMPARE(last_failed, 1);
}

void TestReplayGainScanner::missingFileIsReportedAsFailure() {
  const auto results = runScan(
      {jobFor(dir->path(), {wholeFile(dir->filePath(QStringLiteral("nope.wav")))}, false)});
  QCOMPARE(results.size(), 1);
  QVERIFY(!results.at(0).ok);
  QVERIFY(!results.at(0).error.isEmpty());
}

void TestReplayGainScanner::trackModeLeavesAlbumEmpty() {
  const QString path = sine(QStringLiteral("tone.wav"), 0.5);

  const auto results = runScan({jobFor(dir->path(), {wholeFile(path)}, false)});
  QCOMPARE(results.size(), 1);
  QVERIFY(results.at(0).ok);
  QVERIFY(!results.at(0).gain.has_album);
}

void TestReplayGainScanner::albumGainIsSharedAcrossTheFolder() {
  const QString loud = sine(QStringLiteral("01.wav"), 0.5);
  const QString quiet = sine(QStringLiteral("02.wav"), 0.1);

  const auto results =
      runScan({jobFor(dir->path(), {wholeFile(loud), wholeFile(quiet)}, true)});
  QCOMPARE(results.size(), 2);
  QVERIFY(results.at(0).ok);
  QVERIFY(results.at(1).ok);
  QVERIFY(results.at(0).gain.has_album);
  QVERIFY(results.at(1).gain.has_album);

  QCOMPARE(results.at(0).gain.album_db, results.at(1).gain.album_db);
  QCOMPARE(results.at(0).gain.album_peak, results.at(1).gain.album_peak);

  QVERIFY(std::fabs(results.at(0).gain.track_db - results.at(1).gain.track_db) > 10.0);
  const double album = results.at(0).gain.album_db;
  QVERIFY(std::fabs(album - results.at(0).gain.track_db) < 1.0);
  QVERIFY(std::fabs(album - results.at(1).gain.track_db) > 10.0);
}

void TestReplayGainScanner::cueSlicesAreMeasuredSeparately() {
  const QString path = dir->filePath(QStringLiteral("container.wav"));
  const int seconds = 4;
  QVERIFY(writeWav(path, kRate * seconds, [](int frame) {
    return frame < kRate * 2 ? 0.5 : 0.1;
  }));

  ReplayGain::FileWork work;
  work.path = path;
  work.slices.append(ReplayGain::Slice{0, 2000});
  work.slices.append(ReplayGain::Slice{2000, 2000});

  const auto results = runScan({jobFor(dir->path(), {work}, false)});
  QCOMPARE(results.size(), 2);
  QVERIFY2(results.at(0).ok, qPrintable(results.at(0).error));
  QVERIFY2(results.at(1).ok, qPrintable(results.at(1).error));
  QCOMPARE(results.at(0).begin_ms, 0ULL);
  QCOMPARE(results.at(1).begin_ms, 2000ULL);

  QVERIFY2(results.at(1).gain.track_db - results.at(0).gain.track_db > 10.0,
           qPrintable(QString("%1 vs %2")
                          .arg(results.at(0).gain.track_db)
                          .arg(results.at(1).gain.track_db)));
}

void TestReplayGainScanner::multiSliceFileCannotWriteTags() {
  const QString path = dir->filePath(QStringLiteral("container.wav"));
  QVERIFY(writeWav(path, kRate * 2, [](int) { return 0.5; }));

  ReplayGain::FileWork work;
  work.path = path;
  work.slices.append(ReplayGain::Slice{0, 1000});
  work.slices.append(ReplayGain::Slice{1000, 1000});

  ReplayGain::Job job = jobFor(dir->path(), {work}, false);
  job.write_tags = true;

  const auto results = runScan({job});
  QCOMPARE(results.size(), 2);
  for (const auto &r : results) {
    QVERIFY(r.ok);
    QCOMPARE(r.tag_result, static_cast<int>(ReplayGain::TagResult::Unsupported));
  }
}

void TestReplayGainScanner::emptyRequestFinishesImmediately() {
  ReplayGain::Scanner scanner;
  QSignalSpy done(&scanner, &ReplayGain::Scanner::finished);
  scanner.start({});
  QCOMPARE(done.size(), 1);
  QVERIFY(!scanner.isScanning());
}

void TestReplayGainScanner::progressReachesTotal() {
  const QString first = sine(QStringLiteral("01.wav"), 0.5);
  const QString second = sine(QStringLiteral("02.wav"), 0.3);

  ReplayGain::Scanner scanner;
  QSignalSpy progress(&scanner, &ReplayGain::Scanner::progress);
  QSignalSpy done(&scanner, &ReplayGain::Scanner::finished);

  scanner.start({jobFor(dir->path(), {wholeFile(first), wholeFile(second)}, false)}, 1);
  QVERIFY(done.wait(kScanTimeoutMs));

  QVERIFY(!progress.isEmpty());
  const auto last = progress.last();
  QCOMPARE(last.at(0).toInt(), 2);
  QCOMPARE(last.at(1).toInt(), 2);
}

void TestReplayGainScanner::cancelStopsTheScan() {
  QVector<ReplayGain::FileWork> files;
  for (int i = 0; i < 6; i++) {
    const QString path = sine(QString("%1.wav").arg(i), 0.5, 5);
    QVERIFY(!path.isEmpty());
    files.append(wholeFile(path));
  }

  ReplayGain::Scanner scanner;
  QSignalSpy done(&scanner, &ReplayGain::Scanner::finished);

  scanner.start({jobFor(dir->path(), files, false)}, 1);
  scanner.cancel();
  QVERIFY(done.wait(kScanTimeoutMs));

  QCOMPARE(done.size(), 1);
  QVERIFY(done.first().at(2).toBool());
  QVERIFY(!scanner.isScanning());
}

QTEST_MAIN(TestReplayGainScanner)
#include "tst_replaygainscanner.moc"
