#include <QtTest>
#include <QTemporaryDir>

#include "config/local.h"
#include "modusoperandi.h"
#include "slidingbanner.h"

class TestModusOperandi : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void defaultsToLocalFsWithoutLibraries();
  void startsInMpdWhenTheCurrentLibraryIsMpd();
  void ignoresAnOutOfRangeCurrentLibrary();
  void set_emitsOnlyOnATransition();
  void onLibraryPathChange_derivesTheModeFromTheScheme();
  void onLibraryPathChange_byIndexPersistsAndReturnsThePath();

private:
  QTemporaryDir dir;
  SlidingBanner banner;

  void seed(const QStringList &paths, int current);
};

void TestModusOperandi::init() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestModusOperandi::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
  QFile::remove(dir.filePath("local.yml"));
}

void TestModusOperandi::seed(const QStringList &paths, int current) {
  Config::Local local;
  local.saveLibraryPaths(paths);
  local.saveCurrentLibraryPath(current);
  local.sync();
}

void TestModusOperandi::defaultsToLocalFsWithoutLibraries() {
  Config::Local local;
  ModusOperandi modus(local, &banner);
  QCOMPARE(modus.get(), ModusOperandi::MODUS_LOCALFS);
}

void TestModusOperandi::startsInMpdWhenTheCurrentLibraryIsMpd() {
  seed({"/music", "mpd://localhost:6600"}, 1);

  Config::Local local;
  ModusOperandi modus(local, &banner);
  QCOMPARE(modus.get(), ModusOperandi::MODUS_MPD);
}

void TestModusOperandi::ignoresAnOutOfRangeCurrentLibrary() {
  seed({"mpd://localhost:6600"}, 7);

  Config::Local local;
  ModusOperandi modus(local, &banner);
  QCOMPARE(modus.get(), ModusOperandi::MODUS_LOCALFS);
}

void TestModusOperandi::set_emitsOnlyOnATransition() {
  Config::Local local;
  ModusOperandi modus(local, &banner);
  QSignalSpy spy(&modus, &ModusOperandi::changed);

  modus.set(ModusOperandi::MODUS_LOCALFS);
  QCOMPARE(spy.count(), 0);

  modus.set(ModusOperandi::MODUS_MPD);
  modus.set(ModusOperandi::MODUS_MPD);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toInt(), int(ModusOperandi::MODUS_MPD));
}

void TestModusOperandi::onLibraryPathChange_derivesTheModeFromTheScheme() {
  Config::Local local;
  ModusOperandi modus(local, &banner);

  modus.onLibraryPathChange(QStringLiteral("mpd://localhost:6600"));
  QCOMPARE(modus.get(), ModusOperandi::MODUS_MPD);

  modus.onLibraryPathChange(QStringLiteral("/music"));
  QCOMPARE(modus.get(), ModusOperandi::MODUS_LOCALFS);
}

void TestModusOperandi::onLibraryPathChange_byIndexPersistsAndReturnsThePath() {
  seed({"/music", "/other"}, 0);

  Config::Local local;
  ModusOperandi modus(local, &banner);

  QCOMPARE(modus.onLibraryPathChange(1), QString("/other"));
  QCOMPARE(local.currentLibraryPath(), 1);

  QVERIFY(modus.onLibraryPathChange(9).isEmpty());
  QCOMPARE(local.currentLibraryPath(), 1);
}

QTEST_MAIN(TestModusOperandi)
#include "tst_modusoperandi.moc"
