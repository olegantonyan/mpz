#include <QtTest>

#include "crash_report/crashreport.h"
#include "crash_report/crashlog_format.h"

class TestCrashReport : public QObject {
  Q_OBJECT
private slots:
  void emptyLogIsInvalid();
  void garbageIsInvalid();
  void singleEntryParsed();
  void lastOfManyReturned();
  void entryWithoutTimeIsInvalid();
};

namespace {
  QString entry(const QString &time, const QString &reason, const QString &body) {
    return QString("\n%1\n%2%3\n%4%5\n%6\n\n%7\n%8\n")
      .arg(kCrashBegin)
      .arg(kCrashTimeLabel).arg(time)
      .arg(kCrashReasonLabel).arg(reason)
      .arg(body)
      .arg(kCrashEnd);
  }
}

void TestCrashReport::emptyLogIsInvalid() {
  QVERIFY(!lastCrash(QString()).valid);
  QVERIFY(!lastCrash(QStringLiteral("")).valid);
}

void TestCrashReport::garbageIsInvalid() {
  QVERIFY(!lastCrash(QStringLiteral("nothing to see here\njust logs\n")).valid);
}

void TestCrashReport::singleEntryParsed() {
  const CrashEntry e = lastCrash(entry("1720800000", "SIGSEGV (11)", "frame0\nframe1"));
  QVERIFY(e.valid);
  QCOMPARE(e.id, QStringLiteral("1720800000"));
  QVERIFY(e.text.contains(QStringLiteral("SIGSEGV (11)")));
  QVERIFY(e.text.contains(QLatin1String(kCrashBegin)));
  QVERIFY(e.text.contains(QLatin1String(kCrashEnd)));
}

void TestCrashReport::lastOfManyReturned() {
  const QString log = entry("1000", "SIGABRT (6)", "old frames") +
                      entry("2000", "SIGSEGV (11)", "newer frames") +
                      entry("3000", "SIGBUS (7)", "newest frames");
  const CrashEntry e = lastCrash(log);
  QVERIFY(e.valid);
  QCOMPARE(e.id, QStringLiteral("3000"));
  QVERIFY(e.text.contains(QStringLiteral("newest frames")));
  QVERIFY(e.text.contains(QStringLiteral("SIGBUS (7)")));
  // Must not drag in the earlier entries.
  QVERIFY(!e.text.contains(QStringLiteral("newer frames")));
  QVERIFY(!e.text.contains(QStringLiteral("old frames")));
}

void TestCrashReport::entryWithoutTimeIsInvalid() {
  const QString log = QString("\n%1\n%2SIGSEGV\nno time line here\n%3\n")
    .arg(kCrashBegin).arg(kCrashReasonLabel).arg(kCrashEnd);
  QVERIFY(!lastCrash(log).valid);
}

QTEST_GUILESS_MAIN(TestCrashReport)
#include "tst_crashreport.moc"
