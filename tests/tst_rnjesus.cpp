#include <QtTest>

#include "rnjesus.h"

class TestRNJesus : public QObject {
  Q_OBJECT
private slots:
  void generateSizeOneAlwaysReturnsZero();
  void generateNonPositiveReturnsZero();
  void generateBoundedStaysInRange();
  void generateUnboundedProducesNonZeroSometimes();
};

void TestRNJesus::generateSizeOneAlwaysReturnsZero() {
  for (int i = 0; i < 100; ++i) {
    QCOMPARE(RNJesus::generate(1), quint64{0});
  }
}

void TestRNJesus::generateNonPositiveReturnsZero() {
  QCOMPARE(RNJesus::generate(0),  quint64{0});
  QCOMPARE(RNJesus::generate(-1), quint64{0});
  QCOMPARE(RNJesus::generate(-99), quint64{0});
}

void TestRNJesus::generateBoundedStaysInRange() {
  const int bound = 17;
  for (int i = 0; i < 5000; ++i) {
    quint64 v = RNJesus::generate(bound);
    QVERIFY(v < static_cast<quint64>(bound));
  }
}

void TestRNJesus::generateUnboundedProducesNonZeroSometimes() {
  RNJesus::seed();
  bool saw_nonzero = false;
  for (int i = 0; i < 32; ++i) {
    if (RNJesus::generate() != 0) {
      saw_nonzero = true;
      break;
    }
  }
  QVERIFY(saw_nonzero);
}

QTEST_GUILESS_MAIN(TestRNJesus)
#include "tst_rnjesus.moc"
