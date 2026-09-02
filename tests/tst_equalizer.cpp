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

  void extraGainAppliesWithEqDisabled();
  void extraGainDoesNotRunTheFilters();
  void extraGainLeavesFilterStateIntact();
  void passthroughOnlyWhenBothAreNeutral();
  void positiveExtraGainClampsFloatOutput();
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

  // With auto-preamp, the combined response (preamp included) must not exceed 0 dBFS anywhere: a 0 dBFS input can never clip.
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

void TestEqualizer::extraGainAppliesWithEqDisabled() {
  Equalizer e;
  e.setEnabled(false);
  e.setExtraGainDb(6.0206);
  QVERIFY(!e.isPassthrough());

  std::vector<float> f{0.1f, -0.2f, 0.25f, -0.05f};
  const std::vector<float> before = f;
  e.processFloat(f.data(), f.size(), 1);

  for (std::size_t i = 0; i < f.size(); ++i) {
    QVERIFY2(std::fabs(f[i] - before[i] * 2.0f) < 1e-4,
             qPrintable(QString("%1 -> %2").arg(before[i]).arg(f[i])));
  }
}

void TestEqualizer::extraGainDoesNotRunTheFilters() {
  Equalizer filtered;
  filtered.setBands({mk(Band::Type::Peaking, 1000.0, 12.0, 1.0)});
  filtered.setPreampDb(-6.0);
  filtered.setAutoPreamp(false);
  filtered.setEnabled(false);
  filtered.setExtraGainDb(6.0206);

  Equalizer plain;
  plain.setEnabled(false);
  plain.setExtraGainDb(6.0206);

  const int n = 2048;
  std::vector<float> a(n), b(n);
  for (int i = 0; i < n; ++i) {
    a[i] = b[i] = static_cast<float>(0.25 * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0));
  }
  filtered.processFloat(a.data(), n, 1);
  plain.processFloat(b.data(), n, 1);

  double max_err = 0.0;
  for (int i = 0; i < n; ++i) {
    max_err = std::max(max_err, std::fabs(double(a[i]) - double(b[i])));
  }
  QVERIFY2(max_err < 1e-6, qPrintable(QString("max_err=%1").arg(max_err)));
}

void TestEqualizer::extraGainLeavesFilterStateIntact() {
  const int n = 1024;
  std::vector<float> steady(2 * n), stepped(2 * n);
  for (int i = 0; i < 2 * n; ++i) {
    steady[i] = stepped[i] = static_cast<float>(0.25 * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0));
  }

  auto build = []() {
    Equalizer e;
    e.setBands({mk(Band::Type::Peaking, 1000.0, 9.0, 1.0)});
    e.setAutoPreamp(false);
    e.setPreampDb(0.0);
    e.setEnabled(true);
    return e;
  };

  Equalizer a = build();
  a.processFloat(steady.data(), n, 1);
  a.processFloat(steady.data() + n, n, 1);

  Equalizer b = build();
  b.processFloat(stepped.data(), n, 1);
  b.setExtraGainDb(0.0);
  b.processFloat(stepped.data() + n, n, 1);

  double max_err = 0.0;
  for (int i = n; i < 2 * n; ++i) {
    max_err = std::max(max_err, std::fabs(double(steady[i]) - double(stepped[i])));
  }
  QVERIFY2(max_err < 1e-9, qPrintable(QString("max_err=%1").arg(max_err)));
}

void TestEqualizer::passthroughOnlyWhenBothAreNeutral() {
  Equalizer e;
  e.setEnabled(false);
  QVERIFY(e.isIdentity());
  QVERIFY(e.isPassthrough());

  e.setExtraGainDb(-3.0);
  QVERIFY(e.isIdentity());
  QVERIFY(!e.isPassthrough());

  e.setExtraGainDb(0.0);
  QVERIFY(e.isPassthrough());

  e.setBands({mk(Band::Type::Peaking, 1000.0, 6.0, 1.0)});
  e.setEnabled(true);
  QVERIFY(!e.isIdentity());
  QVERIFY(!e.isPassthrough());
}

void TestEqualizer::positiveExtraGainClampsFloatOutput() {
  Equalizer e;
  e.setEnabled(false);
  e.setExtraGainDb(12.0);

  std::vector<float> f{0.9f, -0.9f, 0.1f};
  e.processFloat(f.data(), f.size(), 1);

  QVERIFY(f[0] <= 1.0f);
  QVERIFY(f[1] >= -1.0f);
  QVERIFY2(std::fabs(f[2] - 0.1f * std::pow(10.0f, 12.0f / 20.0f)) < 1e-4,
           qPrintable(QString::number(f[2])));
}

QTEST_GUILESS_MAIN(TestEqualizer)
#include "tst_equalizer.moc"
