#include <QtTest>

#include "update_check/updatechecker.h"

class TestUpdateChecker : public QObject {
  Q_OBJECT
private slots:
  void remoteNewer();
  void remoteEqualOrOlder();
  void tagPrefixStripped();
  void nonNumericSuffixIgnored();
  void differingComponentCounts();
  void unparseableInputIsNotNewer();
};

void TestUpdateChecker::remoteNewer() {
  QVERIFY(UpdateChecker::isNewer(QStringLiteral("2.0.16"), QStringLiteral("2.0.15")));
  QVERIFY(UpdateChecker::isNewer(QStringLiteral("3.0.0"), QStringLiteral("2.9.9")));
}

void TestUpdateChecker::remoteEqualOrOlder() {
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("2.0.15"), QStringLiteral("2.0.15")));
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("2.0.14"), QStringLiteral("2.0.15")));
}

void TestUpdateChecker::tagPrefixStripped() {
  QVERIFY(UpdateChecker::isNewer(QStringLiteral("v2.1.0"), QStringLiteral("2.0.15")));
  QVERIFY(UpdateChecker::isNewer(QStringLiteral("V2.1.0"), QStringLiteral("v2.0.15")));
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("v2.0.15"), QStringLiteral("2.0.15")));
}

void TestUpdateChecker::nonNumericSuffixIgnored() {
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("2.0.12 [next]"), QStringLiteral("2.0.12")));
  QVERIFY(UpdateChecker::isNewer(QStringLiteral("2.0.13 [next]"), QStringLiteral("2.0.12")));
}

void TestUpdateChecker::differingComponentCounts() {
  QVERIFY(UpdateChecker::isNewer(QStringLiteral("2.1"), QStringLiteral("2.0.9")));
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("2.0"), QStringLiteral("2.0.1")));
}

void TestUpdateChecker::unparseableInputIsNotNewer() {
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("not-a-version"), QStringLiteral("2.0.15")));
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral("2.0.16"), QStringLiteral("")));
  QVERIFY(!UpdateChecker::isNewer(QStringLiteral(""), QStringLiteral("2.0.15")));
}

QTEST_GUILESS_MAIN(TestUpdateChecker)
#include "tst_updatechecker.moc"
