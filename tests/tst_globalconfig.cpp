#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "config/global.h"

class TestGlobalConfig : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void stopWhenTrackRemoved_defaultsFalse();
  void stopWhenTrackRemoved_readsTrue();
  void stopWhenTrackRemoved_readsFalse();

private:
  QTemporaryDir tempDir;
  void writeGlobalYaml(const QByteArray &body);
};

void TestGlobalConfig::init() {
  tempDir.setAutoRemove(true);
  QVERIFY(tempDir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", tempDir.path().toUtf8());
}

void TestGlobalConfig::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestGlobalConfig::writeGlobalYaml(const QByteArray &body) {
  QFile f(tempDir.filePath(QStringLiteral("global.yml")));
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
  f.write(body);
}

void TestGlobalConfig::stopWhenTrackRemoved_defaultsFalse() {
  Config::Global g;
  QCOMPARE(g.stopWhenTrackRemoved(), false);
}

void TestGlobalConfig::stopWhenTrackRemoved_readsTrue() {
  writeGlobalYaml("stop_when_track_removed: true\n");
  Config::Global g;
  QCOMPARE(g.stopWhenTrackRemoved(), true);
}

void TestGlobalConfig::stopWhenTrackRemoved_readsFalse() {
  writeGlobalYaml("stop_when_track_removed: false\n");
  Config::Global g;
  QCOMPARE(g.stopWhenTrackRemoved(), false);
}

QTEST_GUILESS_MAIN(TestGlobalConfig)
#include "tst_globalconfig.moc"
