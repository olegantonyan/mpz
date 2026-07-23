#include <QtTest>

#include "eq/biquad.h"

#include <cmath>

using Eq::Band;
using Eq::Biquad;

class TestEqBiquad : public QObject {
  Q_OBJECT
private slots:
  void identityPassesThrough();
  void peakingGainAtCenter();
  void lowShelfShape();
  void highPassRejectsDc();
};

static double magAtHz(const Biquad &bq, double hz, double fs) {
  return bq.magnitude(2.0 * M_PI * hz / fs);
}

void TestEqBiquad::identityPassesThrough() {
  Biquad bq; // default: b0=1, everything else 0
  QCOMPARE(bq.process(0.5), 0.5);
  QCOMPARE(bq.process(-0.3), -0.3);
  QVERIFY(std::fabs(bq.magnitude(1.0) - 1.0) < 1e-12);
}

void TestEqBiquad::peakingGainAtCenter() {
  Band b;
  b.type = Band::Type::Peaking;
  b.freq_hz = 1000.0;
  b.gain_db = 6.0;
  b.q = 1.0;
  const double fs = 48000.0;
  const Biquad bq = Eq::designBiquad(b, fs);

  const double db = 20.0 * std::log10(magAtHz(bq, 1000.0, fs));
  QVERIFY(std::fabs(db - 6.0) < 0.1);

  // Far from center the peaking filter is unity.
  QVERIFY(std::fabs(magAtHz(bq, 50.0, fs) - 1.0) < 0.05);
}

void TestEqBiquad::lowShelfShape() {
  Band b;
  b.type = Band::Type::LowShelf;
  b.freq_hz = 200.0;
  b.gain_db = 6.0;
  b.q = 0.707;
  const double fs = 48000.0;
  const Biquad bq = Eq::designBiquad(b, fs);

  const double low_db = 20.0 * std::log10(magAtHz(bq, 20.0, fs));
  const double high_db = 20.0 * std::log10(magAtHz(bq, 15000.0, fs));
  QVERIFY(std::fabs(low_db - 6.0) < 0.3);   // boosted below the corner
  QVERIFY(std::fabs(high_db) < 0.3);        // flat above it
}

void TestEqBiquad::highPassRejectsDc() {
  Band b;
  b.type = Band::Type::HighPass;
  b.freq_hz = 100.0;
  b.q = 0.707;
  const double fs = 48000.0;
  const Biquad bq = Eq::designBiquad(b, fs);

  QVERIFY(magAtHz(bq, 10.0, fs) < 0.2);          // subsonic strongly attenuated
  QVERIFY(std::fabs(magAtHz(bq, 15000.0, fs) - 1.0) < 0.05); // passband unity
}

QTEST_GUILESS_MAIN(TestEqBiquad)
#include "tst_eqbiquad.moc"
