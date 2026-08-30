#include "fixture.h"

#include <QHash>
#include <QProcess>

// End-to-end over the real binary: only a second process can exercise the
// hand-off in app/main.cpp. That the handed-over files actually become a
// playlist is asserted in-process by tst_fileopen.
//
// The IPC socket lives under QStandardPaths::CacheLocation, which no environment
// variable relocates on macOS, so these cases synchronize on the app's own log
// output instead of on the socket file, and skip when a real mpz already owns it.
class TestSingleInstance : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();
  void versionFlagPrintsTheVersionAndExits();
  void secondInstanceHandsItsFilesOverAndExits();
  void secondInstanceRunsOnItsOwnWhenSingleInstanceIsOff();

private:
  QTemporaryDir config_dir;
  QString music;
  QList<QProcess *> running;
  QHash<QProcess *, QByteArray> logs;

  void seed(bool single_instance);
  QProcess *launch(const QStringList &args);
  QByteArray logOf(QProcess *proc);
  bool listening(QProcess *proc);
};

void TestSingleInstance::initTestCase() {
  QVERIFY(config_dir.isValid());
  music = config_dir.path() + "/music";
  QVERIFY(GuiTest::copyAudioFixtures(music));
}

void TestSingleInstance::cleanup() {
  for (auto *proc : running) {
    proc->kill();
    proc->waitForFinished(5000);
    delete proc;
  }
  running.clear();
  logs.clear();
  QFile::remove(config_dir.path() + "/global.yml");
  QFile::remove(config_dir.path() + "/local.yml");
}

void TestSingleInstance::seed(bool single_instance) {
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", config_dir.path().toUtf8());
  Config::Global global;
  Config::Local local;
  global.saveSingleInstance(single_instance);
  global.saveDisableAutoUpdateCheck(true);
  local.saveDisableQhotkey(true);
  QVERIFY(global.sync() && local.sync());
}

QProcess *TestSingleInstance::launch(const QStringList &args) {
  auto env = QProcessEnvironment::systemEnvironment();
  env.insert("MPZ_CONFIG_DIR_OVERRIDE", config_dir.path());
  env.insert("QT_QPA_PLATFORM", "offscreen");

  auto *proc = new QProcess;
  // Drain as it arrives: an unread stderr pipe fills up and blocks the child.
  connect(proc, &QProcess::readyReadStandardError, this, [this, proc]() {
    logs[proc].append(proc->readAllStandardError());
  });
  proc->setProcessEnvironment(env);
  proc->setWorkingDirectory(config_dir.path());
  proc->setProgram(QStringLiteral(MPZ_BINARY));
  proc->setArguments(args);
  proc->start();
  running << proc;
  return proc;
}

QByteArray TestSingleInstance::logOf(QProcess *proc) {
  logs[proc].append(proc->readAllStandardError());
  return logs.value(proc);
}

bool TestSingleInstance::listening(QProcess *proc) {
  return logOf(proc).contains("first instance started");
}

void TestSingleInstance::versionFlagPrintsTheVersionAndExits() {
  seed(true);
  auto *proc = launch({"--version"});

  QVERIFY(proc->waitForFinished(30000));
  QCOMPARE(proc->exitCode(), 0);
  QCOMPARE(QString::fromUtf8(proc->readAllStandardOutput()).trimmed(), QStringLiteral(VERSION));
  // Short-circuits before any config or IPC.
  QVERIFY(!logOf(proc).contains("first instance started"));
}

void TestSingleInstance::secondInstanceHandsItsFilesOverAndExits() {
  seed(true);
  auto *first = launch({});
  QVERIFY(first->waitForStarted(30000));
  QTRY_VERIFY_WITH_TIMEOUT(listening(first) || first->state() != QProcess::Running, 30000);
  if (!listening(first)) {
    QSKIP("another mpz instance already owns the single-instance socket");
  }

  auto *second = launch({music});

  QVERIFY(second->waitForFinished(30000));
  QCOMPARE(second->exitCode(), 0);
  QVERIFY(logOf(second).contains("reusing another instance"));
  QCOMPARE(first->state(), QProcess::Running);
}

void TestSingleInstance::secondInstanceRunsOnItsOwnWhenSingleInstanceIsOff() {
  seed(false);
  auto *first = launch({});
  QVERIFY(first->waitForStarted(30000));
  auto *second = launch({music});
  QVERIFY(second->waitForStarted(30000));

  QVERIFY(!second->waitForFinished(3000));
  QCOMPARE(second->state(), QProcess::Running);
  QCOMPARE(first->state(), QProcess::Running);
  QVERIFY(!listening(first));
}

MPZ_GUI_TEST_MAIN(TestSingleInstance)
#include "tst_singleinstance.moc"
