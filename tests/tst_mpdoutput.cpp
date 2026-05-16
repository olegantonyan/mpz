#include <QtTest>

#include "mpd_client/output.h"

using MpdClient::Output;

class TestMpdOutput : public QObject {
  Q_OBJECT
private slots:
  void defaultIsInvalidAndDisabled();
  void nullStructUpdateResetsToInvalid();
  void enumValuesAreStable();
};

void TestMpdOutput::defaultIsInvalidAndDisabled() {
  Output o;
  QVERIFY(!o.isValid());
  QCOMPARE(o.id(),     -1);
  QCOMPARE(o.name(),   QString());
  QCOMPARE(o.state(),  Output::STATE_DISABLED);
  QVERIFY(!o.isEnabled());
}

void TestMpdOutput::nullStructUpdateResetsToInvalid() {
  Output o;
  o.updateFromMpdOutput(nullptr);
  QVERIFY(!o.isValid());
  QCOMPARE(o.id(),    -1);
  QCOMPARE(o.name(),  QString());
  QCOMPARE(o.state(), Output::STATE_DISABLED);
}

void TestMpdOutput::enumValuesAreStable() {
  // The mpd protocol persists output state as 0/1; lock the mapping down.
  QCOMPARE(static_cast<int>(Output::STATE_DISABLED), 0);
  QCOMPARE(static_cast<int>(Output::STATE_ENABLED),  1);
}

QTEST_GUILESS_MAIN(TestMpdOutput)
#include "tst_mpdoutput.moc"
