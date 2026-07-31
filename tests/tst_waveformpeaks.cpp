#include <QtTest>

#include "waveform/peaks.h"

using Waveform::Peaks;

namespace {
  Peaks sample(int count = 4) {
    Peaks p;
    p.bucket_ms = 20;
    p.duration_ms = quint64(count) * 20;
    for (int i = 0; i < count; ++i) {
      p.peak.append(static_cast<quint8>(200 - i));
      p.rms.append(static_cast<quint8>(100 + i));
    }
    return p;
  }
}

class TestWaveformPeaks : public QObject {
  Q_OBJECT
private slots:
  void roundTrip();
  void emptyIsNotSerialized();
  void mismatchedLengthsAreEmpty();
  void zeroBucketIsEmpty();
  void badMagicRejected();
  void badVersionRejected();
  void truncatedBlobRejected();
  void trailingGarbageRejected();
};

void TestWaveformPeaks::roundTrip() {
  const Peaks in = sample(1000);
  const Peaks out = Peaks::deserialize(in.serialize());

  QVERIFY(!out.isEmpty());
  QCOMPARE(out.bucket_ms, in.bucket_ms);
  QCOMPARE(out.duration_ms, in.duration_ms);
  QCOMPARE(out.peak, in.peak);
  QCOMPARE(out.rms, in.rms);
}

void TestWaveformPeaks::emptyIsNotSerialized() {
  QVERIFY(Peaks().isEmpty());
  QVERIFY(Peaks().serialize().isEmpty());
  QVERIFY(Peaks::deserialize(QByteArray()).isEmpty());
}

void TestWaveformPeaks::mismatchedLengthsAreEmpty() {
  Peaks p = sample();
  p.rms.removeLast();
  QVERIFY(p.isEmpty());
  QVERIFY(p.serialize().isEmpty());
}

void TestWaveformPeaks::zeroBucketIsEmpty() {
  Peaks p = sample();
  p.bucket_ms = 0;
  QVERIFY(p.isEmpty());
}

void TestWaveformPeaks::badMagicRejected() {
  QByteArray blob = sample().serialize();
  blob[0] = 'X';
  QVERIFY(Peaks::deserialize(blob).isEmpty());
}

void TestWaveformPeaks::badVersionRejected() {
  QByteArray blob = sample().serialize();
  blob[4] = 99;
  QVERIFY(Peaks::deserialize(blob).isEmpty());
}

void TestWaveformPeaks::truncatedBlobRejected() {
  const QByteArray blob = sample().serialize();
  for (int cut = 1; cut < blob.size(); ++cut) {
    QVERIFY(Peaks::deserialize(blob.left(blob.size() - cut)).isEmpty());
  }
}

void TestWaveformPeaks::trailingGarbageRejected() {
  QVERIFY(Peaks::deserialize(sample().serialize() + "junk").isEmpty());
}

QTEST_GUILESS_MAIN(TestWaveformPeaks)
#include "tst_waveformpeaks.moc"
