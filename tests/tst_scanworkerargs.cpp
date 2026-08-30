#include <QtTest>

#include "replaygain/scanworker.h"

namespace {
  bool invocation(const QList<QByteArray> &args) {
    QList<QByteArray> storage = args;
    QVector<char *> argv;
    for (auto &a : storage) {
      argv << a.data();
    }
    return ReplayGain::isScanWorkerInvocation(argv.size(), argv.data());
  }
}

class TestScanWorkerArgs : public QObject {
  Q_OBJECT
private slots:
  void flagIsStable();
  void detectsTheFlagAtAnyPosition();
  void ignoresArgv0();
  void rejectsNearMisses();
};

void TestScanWorkerArgs::flagIsStable() {
  QCOMPARE(ReplayGain::scanWorkerFlag(), QString("--replaygain-worker"));
}

void TestScanWorkerArgs::detectsTheFlagAtAnyPosition() {
  QVERIFY(invocation({"mpz", "--replaygain-worker"}));
  QVERIFY(invocation({"mpz", "--other", "--replaygain-worker"}));
}

void TestScanWorkerArgs::ignoresArgv0() {
  QVERIFY(!invocation({"--replaygain-worker"}));
}

void TestScanWorkerArgs::rejectsNearMisses() {
  QVERIFY(!invocation({"mpz"}));
  QVERIFY(!invocation({"mpz", "--replaygain-worker=1"}));
  QVERIFY(!invocation({"mpz", "-replaygain-worker"}));
  QVERIFY(!invocation({"mpz", "--ReplayGain-Worker"}));
}

QTEST_GUILESS_MAIN(TestScanWorkerArgs)
#include "tst_scanworkerargs.moc"
