#include <QtTest>
#include <QApplication>
#include <QStyle>
#include <QTemporaryDir>

#include "config/local.h"
#include "modusoperandi.h"
#include "playlist_ui/playlistproxyfiltermodel.h"
#include "slidingbanner.h"

namespace {
  Track mk(const QString &artist, const QString &album, const QString &title, const QString &file) {
    return Track("/music/" + file + ".mp3", 0, artist, album, title, 1, 2000, 1000, 2, 320, 44100);
  }
}

class TestPlaylistProxyFilter : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();
  void startsOnTheLocalfsModel();
  void filter_matchesArtistAlbumFilenameAndTitle();
  void filter_isCaseInsensitive();
  void filter_emptyTermAcceptsEverything();
  void filterAcceptsColumn_neverHidesAColumn();
  void switchTo_swapsTheSourceModel();
  void forwardsReorderAndAppendSignals();

private:
  QTemporaryDir dir;
  PlaylistUi::ColumnsConfig columns;
  SlidingBanner banner;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<PlaylistUi::ProxyFilterModel> proxy;
  std::shared_ptr<Playlist::Playlist> playlist;
};

void TestPlaylistProxyFilter::initTestCase() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestPlaylistProxyFilter::cleanupTestCase() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestPlaylistProxyFilter::init() {
  proxy.reset();
  modus.reset();
  local.reset();

  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  proxy = std::make_unique<PlaylistUi::ProxyFilterModel>(qApp->style(), columns, *modus);

  playlist = std::make_shared<Playlist::Playlist>();
  playlist->load({mk("Pink Floyd", "Animals", "Dogs", "01_dogs"),
                  mk("Tool", "Lateralus", "Schism", "02_schism"),
                  mk("Opeth", "Blackwater Park", "Bleak", "03_bleak")});
  proxy->activeModel()->setPlaylist(playlist);
}

void TestPlaylistProxyFilter::startsOnTheLocalfsModel() {
  QVERIFY(proxy->activeModel() != nullptr);
  QCOMPARE(proxy->rowCount(), 3);
}

void TestPlaylistProxyFilter::filter_matchesArtistAlbumFilenameAndTitle() {
  proxy->filter("Tool");
  QCOMPARE(proxy->rowCount(), 1);

  proxy->filter("Animals");
  QCOMPARE(proxy->rowCount(), 1);

  proxy->filter("03_bleak");
  QCOMPARE(proxy->rowCount(), 1);

  proxy->filter("Schism");
  QCOMPARE(proxy->rowCount(), 1);

  proxy->filter("nothing here");
  QCOMPARE(proxy->rowCount(), 0);
}

void TestPlaylistProxyFilter::filter_isCaseInsensitive() {
  proxy->filter("pInK fLoYd");
  QCOMPARE(proxy->rowCount(), 1);
}

void TestPlaylistProxyFilter::filter_emptyTermAcceptsEverything() {
  proxy->filter("Tool");
  proxy->filter("");
  QCOMPARE(proxy->rowCount(), 3);
}

void TestPlaylistProxyFilter::filterAcceptsColumn_neverHidesAColumn() {
  proxy->filter("Tool");
  QCOMPARE(proxy->columnCount(), proxy->activeModel()->columnCount());
}

void TestPlaylistProxyFilter::switchTo_swapsTheSourceModel() {
#ifdef ENABLE_MPD_SUPPORT
  auto *localfs = proxy->sourceModel();

  modus->set(ModusOperandi::MODUS_MPD);
  QVERIFY(proxy->sourceModel() != localfs);

  modus->set(ModusOperandi::MODUS_LOCALFS);
  QCOMPARE(proxy->sourceModel(), localfs);
#else
  QSKIP("mpd support disabled");
#endif
}

void TestPlaylistProxyFilter::forwardsReorderAndAppendSignals() {
  QSignalSpy reordered(proxy.get(), &PlaylistUi::ProxyFilterModel::tracksReordered);
  QSignalSpy appended(proxy.get(), &PlaylistUi::ProxyFilterModel::appendToPlaylistAsyncFinished);

  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  stream << qint32(1) << qint32(0);
  QMimeData mime;
  mime.setData(QStringLiteral("application/x-mpz-playlist-tracks"), bytes);
  proxy->activeModel()->dropMimeData(&mime, Qt::MoveAction, 3, 0, QModelIndex());
  QCOMPARE(reordered.count(), 1);

  proxy->activeModel()->insertTracks({mk("New", "New", "New", "04_new")}, 0);
  QCOMPARE(appended.count(), 1);
}

QTEST_MAIN(TestPlaylistProxyFilter)
#include "tst_playlistproxyfilter.moc"
