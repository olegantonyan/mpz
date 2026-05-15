#include <QtTest>
#include <QCoreApplication>

#include "sysinfo.h"

class TestSysInfo : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void returnsNonEmptyList();
  void containsExpectedLabels();
  void noEntryIsEmpty();
  void appVersionLineReflectsQCoreApplicationValue();
};

void TestSysInfo::initTestCase() {
  // main.cpp does qApp->setApplicationVersion(VERSION) before SysInfo is ever
  // read; mirror that so the "App version:" line has a value.
  QCoreApplication::setApplicationVersion(QStringLiteral("test-version"));
}

void TestSysInfo::returnsNonEmptyList() {
  const auto info = SysInfo::get();
  QVERIFY(info.size() >= 6);
}

void TestSysInfo::containsExpectedLabels() {
  const auto info = SysInfo::get();
  const QString joined = info.join('\n');
  QVERIFY(joined.contains(QStringLiteral("App version:")));
  QVERIFY(joined.contains(QStringLiteral("Qt version:")));
  QVERIFY(joined.contains(QStringLiteral("Build ABI:")));
  QVERIFY(joined.contains(QStringLiteral("Build CPU architecture:")));
  QVERIFY(joined.contains(QStringLiteral("Current CPU architecture:")));
  QVERIFY(joined.contains(QStringLiteral("Kernel type:")));
  QVERIFY(joined.contains(QStringLiteral("Kernel version:")));
  QVERIFY(joined.contains(QStringLiteral("Product name:")));
}

void TestSysInfo::noEntryIsEmpty() {
  const auto info = SysInfo::get();
  for (const QString &line : info) {
    QVERIFY(!line.isEmpty());
    // Every entry follows "Label: value" — bare "Label:" with nothing after
    // signals a probe that returned empty data.
    QVERIFY2(!line.endsWith(QStringLiteral(": ")),
             qPrintable(QStringLiteral("empty value in '%1'").arg(line)));
  }
}

void TestSysInfo::appVersionLineReflectsQCoreApplicationValue() {
  const auto info = SysInfo::get();
  const QString joined = info.join('\n');
  QVERIFY(joined.contains(QStringLiteral("App version: test-version")));
}

QTEST_GUILESS_MAIN(TestSysInfo)
#include "tst_sysinfo.moc"
