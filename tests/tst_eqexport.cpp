#include <QtTest>

#include "eq/eqexport.h"
#include "eq/autoeqimport.h"

#include <cmath>

using Eq::Band;
using Eq::EqProfile;

class TestEqExport : public QObject {
  Q_OBJECT
private slots:
  void parametricRoundTrips();
  void graphicIsWellFormed();
};

static Band mk(Band::Type t, double f, double g, double q, bool on = true) {
  Band b;
  b.type = t;
  b.freq_hz = f;
  b.gain_db = g;
  b.q = q;
  b.enabled = on;
  return b;
}

void TestEqExport::parametricRoundTrips() {
  EqProfile p;
  p.name = "Test";
  p.auto_preamp = false;
  p.preamp_db = -3.0;
  p.bands << mk(Band::Type::Peaking, 1000, 6.0, 1.0)
          << mk(Band::Type::LowShelf, 100, 3.0, 0.7)
          << mk(Band::Type::HighPass, 30, 0.0, 0.707)
          << mk(Band::Type::Peaking, 5000, -4.0, 2.0, false);

  const QString text = Eq::exportParametricEq(p);
  const Eq::ParsedEq back = Eq::parseParametricEq(text);

  QVERIFY(back.preamp_present);
  QVERIFY(std::fabs(back.preamp_db - (-3.0)) < 0.05);
  QCOMPARE(int(back.bands.size()), 4);

  for (int i = 0; i < 4; ++i) {
    QVERIFY(back.bands[i].type == p.bands[i].type);
    QVERIFY(std::fabs(back.bands[i].freq_hz - p.bands[i].freq_hz) < 0.5);
    QVERIFY(std::fabs(back.bands[i].q - p.bands[i].q) < 0.01);
    QVERIFY(back.bands[i].enabled == p.bands[i].enabled);
  }
  // Gain is meaningful only for the non-pass bands.
  QVERIFY(std::fabs(back.bands[0].gain_db - 6.0) < 0.05);
  QVERIFY(std::fabs(back.bands[1].gain_db - 3.0) < 0.05);
  QVERIFY(std::fabs(back.bands[3].gain_db - (-4.0)) < 0.05);
}

void TestEqExport::graphicIsWellFormed() {
  EqProfile p;
  p.auto_preamp = true;
  p.bands << mk(Band::Type::Peaking, 1000, 6.0, 1.0);

  const QString g = Eq::exportGraphicEq(p, 48000);
  QVERIFY(g.startsWith("GraphicEQ: "));
  QVERIFY(g.contains(';'));

  // Every entry must be "<int hz> <gain>".
  const QString body = g.mid(QString("GraphicEQ: ").size()).trimmed();
  const QStringList pts = body.split(';');
  QVERIFY(pts.size() > 50);
  bool ok = false;
  pts.first().trimmed().section(' ', 0, 0).toInt(&ok);
  QVERIFY(ok);
}

QTEST_GUILESS_MAIN(TestEqExport)
#include "tst_eqexport.moc"
