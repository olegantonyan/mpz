#include <QtTest>
#include <QTemporaryDir>

#include "config/local.h"
#include "modusoperandi.h"
#include "playlists_ui/playlistsproxyfiltermodel.h"
#include "slidingbanner.h"

namespace {
  std::shared_ptr<Playlist::Playlist> mkPlaylist(const QString &name) {
    auto pl = std::make_shared<Playlist::Playlist>();
    pl->rename(name);
    return pl;
  }
}

class TestPlaylistsProxyFilter : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();
  void rowCount_bypassesTheProxyWithoutAFilter();
  void rowCount_defersToTheProxyWhenFiltering();
  void append_returnsAProxyIndex();
  void itemAt_mapsThroughTheProxy();
  void switchTo_localfsReloadsFromConfig();
  void onRename_isANoOpOutsideMpd();

private:
  QTemporaryDir dir;
  SlidingBanner banner;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<PlaylistsUi::ProxyFilterModel> proxy;
};

void TestPlaylistsProxyFilter::initTestCase() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestPlaylistsProxyFilter::cleanupTestCase() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestPlaylistsProxyFilter::init() {
  proxy.reset();
  modus.reset();
  local.reset();
  QFile::remove(dir.filePath("local.yml"));

  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  proxy = std::make_unique<PlaylistsUi::ProxyFilterModel>(*local, *modus);
  // The constructor kicks off a load off the GUI thread; let it land before
  // appending, or its model reset wipes what we just added.
  QSignalSpy loaded(proxy.get(), &PlaylistsUi::ProxyFilterModel::asyncLoadFinished);
  QVERIFY(loaded.wait());

  proxy->append(mkPlaylist("rock"));
  proxy->append(mkPlaylist("jazz"));
}

void TestPlaylistsProxyFilter::rowCount_bypassesTheProxyWithoutAFilter() {
  QCOMPARE(proxy->rowCount(), 2);
  QCOMPARE(proxy->rowCount(), proxy->activeModel()->rowCount());
}

void TestPlaylistsProxyFilter::rowCount_defersToTheProxyWhenFiltering() {
  proxy->setFilterFixedString("rock");
  QCOMPARE(proxy->rowCount(), 1);

  proxy->setFilterFixedString(QString());
  QCOMPARE(proxy->rowCount(), 2);
}

void TestPlaylistsProxyFilter::append_returnsAProxyIndex() {
  const QModelIndex idx = proxy->append(mkPlaylist("blues"));
  QVERIFY(idx.isValid());
  QCOMPARE(idx.model(), proxy.get());
  QCOMPARE(proxy->itemAt(idx)->name(), QString("blues"));
}

void TestPlaylistsProxyFilter::itemAt_mapsThroughTheProxy() {
  proxy->setFilterFixedString("jazz");
  QCOMPARE(proxy->itemAt(proxy->index(0, 0))->name(), QString("jazz"));
}

void TestPlaylistsProxyFilter::switchTo_localfsReloadsFromConfig() {
#ifdef ENABLE_MPD_SUPPORT
  QVERIFY(proxy->persist());
  QSignalSpy spy(proxy.get(), &PlaylistsUi::ProxyFilterModel::asyncLoadFinished);

  modus->set(ModusOperandi::MODUS_MPD);
  modus->set(ModusOperandi::MODUS_LOCALFS);

  QVERIFY(spy.wait());
  QCOMPARE(proxy->rowCount(), 2);
#else
  QSKIP("mpd support disabled");
#endif
}

void TestPlaylistsProxyFilter::onRename_isANoOpOutsideMpd() {
  proxy->onRename("rock", "stone");
  QCOMPARE(proxy->activeModel()->itemList().first()->name(), QString("rock"));
}

QTEST_MAIN(TestPlaylistsProxyFilter)
#include "tst_playlistsproxyfilter.moc"
