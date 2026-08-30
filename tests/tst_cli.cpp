#include <QtTest>
#include <QStandardPaths>

#include "cli.h"

namespace {
  QStringList parse(const QList<QByteArray> &argv) {
    QList<QByteArray> storage = argv;
    QVector<char *> raw;
    for (auto &a : storage) {
      raw << a.data();
    }
    return Cli::arguments(raw.size(), raw.data());
  }
}

class TestCli : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void arguments_dropTheBinaryAndKeepOrder();
  void isVersionRequest_matchesTheExactFlagOnly();
  void claimInstance_runsWhenSingleInstanceIsOff();
  void claimInstance_claimsAFreeSocketAndRuns();
};

void TestCli::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
}

void TestCli::arguments_dropTheBinaryAndKeepOrder() {
  QCOMPARE(parse({"mpz"}), QStringList());
  QCOMPARE(parse({"mpz", "/a.mp3", "/b.flac"}), QStringList({"/a.mp3", "/b.flac"}));
}

void TestCli::isVersionRequest_matchesTheExactFlagOnly() {
  QVERIFY(Cli::isVersionRequest({"--version"}));
  QVERIFY(!Cli::isVersionRequest({}));
  QVERIFY(!Cli::isVersionRequest({"--version", "/a.mp3"}));
  QVERIFY(!Cli::isVersionRequest({"-version"}));
  QVERIFY(!Cli::isVersionRequest({"/a.mp3"}));
}

void TestCli::claimInstance_runsWhenSingleInstanceIsOff() {
  IPC::Instance instance(100);
  QCOMPARE(Cli::claimInstance(instance, false, {"/a.mp3"}), Cli::Startup::Run);
}

void TestCli::claimInstance_claimsAFreeSocketAndRuns() {
  IPC::Instance instance(100);
  QCOMPARE(Cli::claimInstance(instance, true, {"/a.mp3"}), Cli::Startup::Run);
  // Claimed: a fresh probe now finds this process listening.
  QCOMPARE(IPC::Instance(100).anotherPid() > 0, false);
}

QTEST_GUILESS_MAIN(TestCli)
#include "tst_cli.moc"
