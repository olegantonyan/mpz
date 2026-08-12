#include <QtTest>

#include "dynamic_range/dr.h"

#include <cmath>

using DynamicRange::Accumulator;
using DynamicRange::Result;

namespace {
  constexpr int RATE = 48000;
  constexpr qint64 BLOCK = qint64(RATE) * 3;
  constexpr double PI = 3.14159265358979323846;

  QVector<double> pulse(double amp, double duty, qint64 len) {
    QVector<double> v(len, 0.0);
    const qint64 on = qint64(double(len) * duty);
    for (qint64 i = 0; i < on; ++i) {
      v[i] = amp;
    }
    return v;
  }

  QVector<double> repeated(const QVector<double> &block, int times) {
    QVector<double> v;
    v.reserve(block.size() * times);
    for (int i = 0; i < times; ++i) {
      v += block;
    }
    return v;
  }

  QVector<double> sine(double amp, qint64 frames, double freq) {
    QVector<double> v(frames);
    for (qint64 i = 0; i < frames; ++i) {
      v[i] = amp * std::sin(2.0 * PI * freq * double(i) / double(RATE));
    }
    return v;
  }

  QVector<double> square(double amp, qint64 frames, qint64 period) {
    QVector<double> v(frames);
    for (qint64 i = 0; i < frames; ++i) {
      v[i] = (i % period) < (period / 2) ? amp : -amp;
    }
    return v;
  }

  QVector<double> interleave(const QVector<double> &l, const QVector<double> &r) {
    QVector<double> v(l.size() * 2);
    for (qint64 i = 0; i < l.size(); ++i) {
      v[i * 2] = l.at(i);
      v[i * 2 + 1] = r.at(i);
    }
    return v;
  }

  Result mono(const QVector<double> &samples) {
    Accumulator a(1, RATE);
    a.addInterleaved(samples.constData(), samples.size());
    return a.finish();
  }

  Result stereo(const QVector<double> &interleaved) {
    Accumulator a(2, RATE);
    a.addInterleaved(interleaved.constData(), interleaved.size() / 2);
    return a.finish();
  }

  Result valid(double dr) {
    Result r;
    r.valid = true;
    r.dr = dr;
    return r;
  }
}

#define VERIFY_NEAR(actual, expected)                                                    \
  QVERIFY2(std::fabs((actual) - (expected)) < 0.01,                                      \
           qPrintable(QString("expected %1, got %2").arg(expected).arg(actual)))

class TestDynamicRange : public QObject {
  Q_OBJECT
private slots:
  void fullScaleSineIsCalibrated();
  void scaleInvariant();
  void squareWaveClampsToZero();
  void pulseTrainHasExactDr();
  void usesOnlyLoudestBlocks();
  void usesSecondHighestBlockPeak();
  void averagesChannels();
  void trackShorterThanOneBlock();
  void silenceIsValid();
  void dropsTrailingPartialBlock();
  void albumValueRoundsMean();
};

// A: the x2 factor in rms = sqrt(2 * mean(x^2)) puts a full-scale sine at 0 dB
void TestDynamicRange::fullScaleSineIsCalibrated() {
  const Result r = mono(sine(1.0, BLOCK * 10, 1000.0));
  QVERIFY(r.valid);
  VERIFY_NEAR(r.peak_db, 0.0);
  VERIFY_NEAR(r.rms_db, 0.0);
  QCOMPARE(DynamicRange::displayDr(r), 0);
}

void TestDynamicRange::scaleInvariant() {
  const Result r = mono(sine(0.5, BLOCK * 10, 1000.0));
  QVERIFY(r.valid);
  VERIFY_NEAR(r.peak_db, -6.02);
  VERIFY_NEAR(r.rms_db, -6.02);
  QCOMPARE(DynamicRange::displayDr(r), 0);
}

void TestDynamicRange::squareWaveClampsToZero() {
  const Result r = mono(square(1.0, BLOCK * 10, 48));
  QVERIFY(r.valid);
  VERIFY_NEAR(r.peak_db, 0.0);
  VERIFY_NEAR(r.rms_db, 3.01);
  VERIFY_NEAR(r.dr, -3.01);
  QCOMPARE(DynamicRange::displayDr(r), 0);
}

void TestDynamicRange::pulseTrainHasExactDr() {
  const Result r = mono(repeated(pulse(1.0, 0.125, BLOCK), 10));
  QVERIFY(r.valid);
  VERIFY_NEAR(r.peak_db, 0.0);
  VERIFY_NEAR(r.rms_db, -6.02);
  VERIFY_NEAR(r.dr, 6.02);
  QCOMPARE(DynamicRange::displayDr(r), 6);
}

// F: averaging every block instead of the top 20% would give DR13
void TestDynamicRange::usesOnlyLoudestBlocks() {
  QVector<double> samples = repeated(pulse(1.0, 0.125, BLOCK), 2);
  samples += repeated(pulse(0.1, 0.125, BLOCK), 8);
  const Result r = mono(samples);
  QVERIFY(r.valid);
  QCOMPARE(DynamicRange::displayDr(r), 6);
}

// G: using the highest block peak instead of the second highest would give DR12
void TestDynamicRange::usesSecondHighestBlockPeak() {
  QVector<double> samples = repeated(pulse(0.5, 0.125, BLOCK), 10);
  samples[0] = 1.0;
  const Result r = mono(samples);
  QVERIFY(r.valid);
  VERIFY_NEAR(r.peak_db, 0.0);
  QCOMPARE(DynamicRange::displayDr(r), 6);
}

void TestDynamicRange::averagesChannels() {
  const QVector<double> left = repeated(pulse(1.0, 0.125, BLOCK), 10);
  const QVector<double> right = repeated(pulse(1.0, 0.5, BLOCK), 10);
  const Result r = stereo(interleave(left, right));
  QVERIFY(r.valid);
  QCOMPARE(r.channels, 2);
  VERIFY_NEAR(r.peak_db, 0.0);
  VERIFY_NEAR(r.rms_db, -3.01);
  VERIFY_NEAR(r.dr, 3.01);
  QCOMPARE(DynamicRange::displayDr(r), 3);
}

void TestDynamicRange::trackShorterThanOneBlock() {
  const Result r = mono(pulse(1.0, 0.125, RATE * 2));
  QVERIFY(r.valid);
  QCOMPARE(DynamicRange::displayDr(r), 6);
}

void TestDynamicRange::silenceIsValid() {
  const Result r = mono(QVector<double>(BLOCK * 3, 0.0));
  QVERIFY(r.valid);
  QCOMPARE(DynamicRange::displayDr(r), 0);
  QCOMPARE(r.peak_db, DynamicRange::MINUS_INF_DB);
  QCOMPARE(r.rms_db, DynamicRange::MINUS_INF_DB);
}

// K: keeping the trailing partial block would give DR0
void TestDynamicRange::dropsTrailingPartialBlock() {
  QVector<double> samples = repeated(pulse(0.5, 0.125, BLOCK), 10);
  samples += square(1.0, BLOCK / 2, 48);
  const Result r = mono(samples);
  QVERIFY(r.valid);
  QCOMPARE(DynamicRange::displayDr(r), 6);
}

void TestDynamicRange::albumValueRoundsMean() {
  QCOMPARE(DynamicRange::officialAlbumDr({valid(6.0), valid(7.0), valid(8.0)}), 7);
  QCOMPARE(DynamicRange::officialAlbumDr({valid(6.4), valid(6.4)}), 6);
  QCOMPARE(DynamicRange::officialAlbumDr({valid(6.0), Result(), valid(8.0)}), 7);
  QCOMPARE(DynamicRange::officialAlbumDr({}), 0);
}

QTEST_GUILESS_MAIN(TestDynamicRange)
#include "tst_dynamicrange.moc"
