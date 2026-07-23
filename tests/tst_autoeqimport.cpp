#include <QtTest>

#include "eq/autoeqimport.h"

#include <cmath>

using Eq::Band;

class TestAutoEqImport : public QObject {
  Q_OBJECT
private slots:
  void parsesOratoryStyle();
  void skipsUnknownAndKeepsOffState();
  void emptyOnGarbage();
};

void TestAutoEqImport::parsesOratoryStyle() {
  const QString text =
    "Preamp: -6.5 dB\n"
    "Filter 1: ON PK Fc 105 Hz Gain 5.5 dB Q 0.70\n"
    "Filter 2: ON LSC Fc 80 Hz Gain 3.0 dB Q 0.70\n"
    "Filter 3: ON HS Fc 10000 Hz Gain -2.0 dB Q 0.70\n";

  const Eq::ParsedEq p = Eq::parseParametricEq(text);
  QVERIFY(p.preamp_present);
  QVERIFY(std::fabs(p.preamp_db - (-6.5)) < 1e-9);
  QCOMPARE(int(p.bands.size()), 3);

  QVERIFY(p.bands[0].type == Band::Type::Peaking);
  QVERIFY(std::fabs(p.bands[0].freq_hz - 105.0) < 1e-9);
  QVERIFY(std::fabs(p.bands[0].gain_db - 5.5) < 1e-9);
  QVERIFY(std::fabs(p.bands[0].q - 0.70) < 1e-9);

  QVERIFY(p.bands[1].type == Band::Type::LowShelf);
  QVERIFY(p.bands[2].type == Band::Type::HighShelf);
  QVERIFY(std::fabs(p.bands[2].gain_db - (-2.0)) < 1e-9);
}

void TestAutoEqImport::skipsUnknownAndKeepsOffState() {
  const QString text =
    "Filter 1: ON None\n"
    "Filter 2: OFF PK Fc 2000 Hz Gain 4.0 dB Q 2.0\n"
    "Filter 3: ON LP Fc 20000 Hz\n";

  const Eq::ParsedEq p = Eq::parseParametricEq(text);
  QVERIFY(!p.preamp_present);
  QCOMPARE(int(p.bands.size()), 2); // None skipped, the other two kept

  QVERIFY(!p.bands[0].enabled);     // OFF preserved
  QVERIFY(p.bands[0].type == Band::Type::Peaking);

  QVERIFY(p.bands[1].type == Band::Type::LowPass);
  QVERIFY(std::fabs(p.bands[1].gain_db) < 1e-9);   // no gain token
  QVERIFY(std::fabs(p.bands[1].q - 0.707) < 1e-3); // default Q
}

void TestAutoEqImport::emptyOnGarbage() {
  const Eq::ParsedEq p = Eq::parseParametricEq("this is not a preset\n\n# comment\n");
  QVERIFY(p.empty());
}

QTEST_GUILESS_MAIN(TestAutoEqImport)
#include "tst_autoeqimport.moc"
