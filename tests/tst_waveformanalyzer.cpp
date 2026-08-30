#include <QtTest>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "diskcache.h"
#include "waveform/analyzer.h"

class TestWaveformAnalyzer : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void decodesAFileIntoBuckets();
  void servesASecondRequestFromTheDiskCache();
  void repeatingTheSameRequestIsANoOp();
  void missingPathEmitsNothing();
  void cancelStopsFurtherEmissions();
  void diskCacheTrimKeepsTheNewest();

private:
  QTemporaryDir dir;
  QString flac;

  static Waveform::Peaks lastPeaks(const QSignalSpy &spy);
  // Partial results are emitted while decoding; settle on the final one.
  static void waitForFinalReady(QSignalSpy &spy);
};

void TestWaveformAnalyzer::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QVERIFY(dir.isValid());
  flac = dir.filePath("silence.flac");
  QVERIFY(QFile::copy(QStringLiteral(AUDIO_FIXTURES_DIR) + "/silence.flac", flac));
  DiskCache::Store("waveform").clear();
}

Waveform::Peaks TestWaveformAnalyzer::lastPeaks(const QSignalSpy &spy) {
  return spy.last().at(1).value<Waveform::Peaks>();
}

void TestWaveformAnalyzer::waitForFinalReady(QSignalSpy &spy) {
  if (spy.isEmpty()) {
    QVERIFY(spy.wait(30000)); // a cache hit has already emitted synchronously
  }
  int seen = 0;
  do {
    seen = spy.count();
    QTest::qWait(300);
  } while (spy.count() != seen);
}

void TestWaveformAnalyzer::decodesAFileIntoBuckets() {
  Waveform::Analyzer analyzer;
  QSignalSpy spy(&analyzer, &Waveform::Analyzer::ready);

  analyzer.request(flac);

  waitForFinalReady(spy);
  const Waveform::Peaks peaks = lastPeaks(spy);
  QVERIFY(!peaks.isEmpty());
  QCOMPARE(spy.last().first().toString(), flac);
  QCOMPARE(peaks.bucket_ms, quint16(20));
  QCOMPARE(peaks.peak.size(), peaks.rms.size());
  QCOMPARE(peaks.duration_ms, quint64(peaks.peak.size()) * peaks.bucket_ms);
  for (int i = 0; i < peaks.peak.size(); i++) {
    QVERIFY(peaks.peak.at(i) >= peaks.rms.at(i));
  }
}

void TestWaveformAnalyzer::servesASecondRequestFromTheDiskCache() {
  {
    Waveform::Analyzer warm;
    QSignalSpy warming(&warm, &Waveform::Analyzer::ready);
    warm.request(flac);
    waitForFinalReady(warming);
  }

  Waveform::Analyzer analyzer;
  QSignalSpy spy(&analyzer, &Waveform::Analyzer::ready);

  analyzer.request(flac);

  // Served from the cache: emitted synchronously, without spinning the loop.
  QCOMPARE(spy.count(), 1);
  QVERIFY(!lastPeaks(spy).isEmpty());
}

void TestWaveformAnalyzer::repeatingTheSameRequestIsANoOp() {
  Waveform::Analyzer analyzer;
  analyzer.request(flac);
  QSignalSpy spy(&analyzer, &Waveform::Analyzer::ready);

  analyzer.request(flac);

  QCOMPARE(spy.count(), 0);
}

void TestWaveformAnalyzer::missingPathEmitsNothing() {
  Waveform::Analyzer analyzer;
  QSignalSpy spy(&analyzer, &Waveform::Analyzer::ready);

  analyzer.request(QString());
  analyzer.request(dir.filePath("nope.flac"));
  analyzer.request(dir.path());

  QTest::qWait(200);
  QCOMPARE(spy.count(), 0);
}

void TestWaveformAnalyzer::cancelStopsFurtherEmissions() {
  Waveform::Analyzer analyzer;
  analyzer.request(flac);
  analyzer.cancel();
  QSignalSpy spy(&analyzer, &Waveform::Analyzer::ready);

  QTest::qWait(300);
  QCOMPARE(spy.count(), 0);
}

void TestWaveformAnalyzer::diskCacheTrimKeepsTheNewest() {
  DiskCache::Store store("waveform-trim");
  store.clear();
  for (const QString &key : {"a", "b", "c"}) {
    QVERIFY(!store.write(key, "wf", key.toUtf8()).isEmpty());
    QTest::qWait(1100); // mtime resolution
  }

  store.trim({"wf"}, 2);

  QVERIFY(store.find("a", {"wf"}).isEmpty());
  QVERIFY(!store.find("c", {"wf"}).isEmpty());
}

QTEST_GUILESS_MAIN(TestWaveformAnalyzer)
#include "tst_waveformanalyzer.moc"
