#include <QtTest>
#include <QDataStream>
#include <QFile>
#include <QTemporaryDir>

#include <cmath>

#include "dynamic_range/scanner.h"

namespace {
  const int kRate = 44100;
  const int kChannels = 2;
  const int kTimeoutMs = 30000;

  bool writeWav(const QString &path, int frames) {
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
      // Quiet first half, loud second half, so the two halves score differently.
      const double amp = i < frames / 2 ? 0.05 : 0.7;
      const double v = amp * std::sin(2.0 * M_PI * 997.0 * i / kRate);
      const qint16 s = static_cast<qint16>(std::lround(std::clamp(v, -1.0, 1.0) * 32767.0));
      out << s << s;
    }
    return true;
  }

  DynamicRange::Segment segment(quint64 uid, qint64 begin_us, qint64 end_us) {
    DynamicRange::Segment s;
    s.uid = uid;
    s.begin_us = begin_us;
    s.end_us = end_us;
    s.duration_us = end_us < 0 ? 0 : end_us - begin_us;
    return s;
  }
}

class TestDrScanner : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void scansAWholeFile();
  void splitsOneDecodePassIntoSegments();
  void reportsMonotonicProgressAndFinishesOnce();
  void cancelStopsTheScan();
  void undecodablePathYieldsAnInvalidResult();
  void emptyQueueFinishesImmediately();

private:
  QTemporaryDir dir;
  QString wav;
};

void TestDrScanner::initTestCase() {
  QVERIFY(dir.isValid());
  wav = dir.filePath("tone.wav");
  QVERIFY(writeWav(wav, kRate * 4));
}

void TestDrScanner::scansAWholeFile() {
  DynamicRange::Scanner scanner;
  QSignalSpy done(&scanner, &DynamicRange::Scanner::segmentDone);
  QSignalSpy finished(&scanner, &DynamicRange::Scanner::finished);

  scanner.run({DynamicRange::Job{wav, {segment(1, 0, -1)}}});

  QVERIFY(finished.wait(kTimeoutMs));
  QCOMPARE(finished.first().first().toBool(), false);
  QCOMPARE(done.count(), 1);
  QCOMPARE(done.first().first().toULongLong(), 1u);
  const auto result = done.first().at(1).value<DynamicRange::Result>();
  QVERIFY(result.valid);
  QCOMPARE(result.channels, kChannels);
  QCOMPARE(result.sample_rate, kRate);
  QVERIFY(result.frames > 0);
}

void TestDrScanner::splitsOneDecodePassIntoSegments() {
  DynamicRange::Scanner scanner;
  QSignalSpy done(&scanner, &DynamicRange::Scanner::segmentDone);
  QSignalSpy finished(&scanner, &DynamicRange::Scanner::finished);

  // The CUE shape: one file, two tracks.
  scanner.run({DynamicRange::Job{wav, {segment(1, 0, 2000000), segment(2, 2000000, -1)}}});

  QVERIFY(finished.wait(kTimeoutMs));
  QCOMPARE(done.count(), 2);

  QVector<quint64> uids;
  for (const auto &call : done) {
    uids << call.first().toULongLong();
    QVERIFY(call.at(1).value<DynamicRange::Result>().valid);
  }
  std::sort(uids.begin(), uids.end());
  QCOMPARE(uids, QVector<quint64>({1, 2}));

  const auto quiet = done.at(0).at(1).value<DynamicRange::Result>();
  const auto loud = done.at(1).at(1).value<DynamicRange::Result>();
  QVERIFY(loud.peak_db > quiet.peak_db);
}

void TestDrScanner::reportsMonotonicProgressAndFinishesOnce() {
  DynamicRange::Scanner scanner;
  QSignalSpy progress(&scanner, &DynamicRange::Scanner::progress);
  QSignalSpy finished(&scanner, &DynamicRange::Scanner::finished);

  scanner.run({DynamicRange::Job{wav, {segment(1, 0, -1)}}});
  QVERIFY(finished.wait(kTimeoutMs));

  QCOMPARE(finished.count(), 1);
  QVERIFY(!progress.isEmpty());
  qint64 previous = -1;
  for (const auto &call : progress) {
    const qint64 done_us = call.first().toLongLong();
    const qint64 total_us = call.at(1).toLongLong();
    QVERIFY(done_us >= previous);
    QVERIFY(done_us <= total_us);
    previous = done_us;
  }
}

void TestDrScanner::cancelStopsTheScan() {
  DynamicRange::Scanner scanner;
  QSignalSpy done(&scanner, &DynamicRange::Scanner::segmentDone);
  QSignalSpy finished(&scanner, &DynamicRange::Scanner::finished);

  scanner.run({DynamicRange::Job{wav, {segment(1, 0, -1)}}});
  scanner.cancel();

  QVERIFY(finished.wait(kTimeoutMs));
  QCOMPARE(finished.first().first().toBool(), true);
  const int settled = done.count();
  QTest::qWait(300);
  QCOMPARE(done.count(), settled);
}

void TestDrScanner::undecodablePathYieldsAnInvalidResult() {
  DynamicRange::Scanner scanner;
  QSignalSpy done(&scanner, &DynamicRange::Scanner::segmentDone);
  QSignalSpy finished(&scanner, &DynamicRange::Scanner::finished);

  scanner.run({DynamicRange::Job{dir.filePath("nope.wav"), {segment(1, 0, -1)}}});

  QVERIFY(finished.wait(kTimeoutMs));
  QCOMPARE(finished.first().first().toBool(), false);
  // Every segment still reports, so the caller's rows resolve instead of hanging.
  QCOMPARE(done.count(), 1);
  QVERIFY(!done.first().at(1).value<DynamicRange::Result>().valid);
}

void TestDrScanner::emptyQueueFinishesImmediately() {
  DynamicRange::Scanner scanner;
  QSignalSpy finished(&scanner, &DynamicRange::Scanner::finished);

  scanner.run({});

  QVERIFY(finished.wait(kTimeoutMs));
  QCOMPARE(finished.first().first().toBool(), false);
}

QTEST_GUILESS_MAIN(TestDrScanner)
#include "tst_drscanner.moc"
