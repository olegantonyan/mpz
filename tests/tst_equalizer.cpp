#include <QtTest>

#include "eq/equalizer.h"

#include <cmath>
#include <cstring>
#include <vector>

using Eq::Band;
using Eq::Equalizer;

class TestEqualizer : public QObject {
  Q_OBJECT
private slots:
  void disabledIsIdentity();
  void flatIsIdentity();
  void identityLeavesFloatBitExact();
  void bandGainShowsInResponse();
  void autoPreampNeverClips();
  void int16MatchesFloat();
};

static Band mk(Band::Type t, double f, double g, double q) {
  Band b;
  b.type = t;
  b.freq_hz = f;
  b.gain_db = g;
  b.q = q;
  b.enabled = true;
  return b;
}

void TestEqualizer::disabledIsIdentity() {
  Equalizer eq;
  eq.setSampleRate(48000);
  eq.setBands({mk(Band::Type::Peaking, 1000, 6, 1)});
  eq.setEnabled(false);
  QVERIFY(eq.isIdentity());
}

void TestEqualizer::flatIsIdentity() {
  Equalizer eq;
  eq.setSampleRate(48000);
  eq.setEnabled(true);
  eq.setBands({}); // no bands
  eq.setPreampDb(0.0);
  eq.setAutoPreamp(false);
  QVERIFY(eq.isIdentity());
}

void TestEqualizer::identityLeavesFloatBitExact() {
  Equalizer eq;
  eq.setSampleRate(48000);
  eq.setEnabled(true);
  eq.setAutoPreamp(false);
  eq.setPreampDb(0.0);

  float data[6] = {0.1f, -0.2f, 0.33f, -0.44f, 0.5f, -0.6f};
  float copy[6];
  std::memcpy(copy, data, sizeof(data));
  eq.processFloat(data, 3, 2);
  for (int i = 0; i < 6; ++i) {
    QCOMPARE(data[i], copy[i]);
  }
}

void TestEqualizer::bandGainShowsInResponse() {
  Equalizer eq;
  eq.setSampleRate(48000);
  eq.setEnabled(true);
  eq.setAutoPreamp(false);
  eq.setPreampDb(0.0);
  eq.setBands({mk(Band::Type::Peaking, 1000, 6, 1)});

  QVERIFY(std::fabs(eq.magnitudeResponseDb(1000.0) - 6.0) < 0.15);
}

void TestEqualizer::autoPreampNeverClips() {
  Equalizer eq;
  eq.setSampleRate(48000);
  eq.setEnabled(true);
  eq.setAutoPreamp(true);
  eq.setBands({
    mk(Band::Type::Peaking, 100, 8, 1.0),
    mk(Band::Type::Peaking, 1000, 6, 1.5),
    mk(Band::Type::HighShelf, 8000, 5, 0.707),
  });

  // With auto-preamp, the combined response (preamp included) must not exceed
  // 0 dBFS anywhere: a 0 dBFS input can never clip.
  double peak = -100.0;
  for (double f = 20.0; f <= 20000.0; f *= 1.02) {
    peak = std::max(peak, eq.magnitudeResponseDb(f));
  }
  QVERIFY2(peak <= 0.3, qPrintable(QString("peak=%1 dB").arg(peak)));
}

void TestEqualizer::int16MatchesFloat() {
  const std::vector<Band> bands = {mk(Band::Type::Peaking, 1000, 6, 1)};

  Equalizer ef;
  ef.setSampleRate(48000);
  ef.setEnabled(true);
  ef.setAutoPreamp(false);
  ef.setPreampDb(0.0);
  ef.setBands(bands);

  Equalizer ei;
  ei.setSampleRate(48000);
  ei.setEnabled(true);
  ei.setAutoPreamp(false);
  ei.setPreampDb(0.0);
  ei.setBands(bands);

  const int n = 2048;
  std::vector<float> f(n);
  std::vector<int16_t> s(n);
  for (int i = 0; i < n; ++i) {
    const double x = 0.5 * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0);
    f[i] = static_cast<float>(x);
    s[i] = static_cast<int16_t>(std::lround(x * 32768.0));
  }
  ef.processFloat(f.data(), n, 1);
  ei.processInt16(s.data(), n, 1);

  // Compare the settled tail; allow for quantization + dither (a few LSB).
  double max_err = 0.0;
  for (int i = 512; i < n; ++i) {
    const double si = s[i] / 32768.0;
    max_err = std::max(max_err, std::fabs(si - f[i]));
  }
  QVERIFY2(max_err < 0.01, qPrintable(QString("max_err=%1").arg(max_err)));
}

QTEST_GUILESS_MAIN(TestEqualizer)
#include "tst_equalizer.moc"
