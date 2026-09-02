#include "fixture.h"
#include "server.h"

#include "config/global.h"
#include "config/local.h"
#include "directory_ui/directorymodel/mpd.h"
#include "modusoperandi.h"
#include "slidingbanner.h"
#include "mpd_client/client.h"
#include "playlist/mpdloader.h"
#include "playlists_ui/mpd/playlistsmodel.h"

#include <QSignalSpy>
#include <QThreadPool>
#include <QtTest>

// Models and loaders over a real server. Config::Global, Config::Local and ModusOperandi are single-instance
// guarded, so one stack is built for the whole binary and isolation comes from resetting the server between cases.
class TestMpdModels : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void cleanup();

  void modusSwitchesToMpdForAnMpdLibraryPath();
  void mpdReadyIsEmittedWhenTheConnectionOpens();
  void directoryTreeMirrorsTheServerRoot();
  void directoryTreeFetchesChildrenOnDemand();
  void directoryTreeFiltersTopLevelEntries();
  void directoryTreeSortsDirectoriesFirst();
  void databaseUpdateRebuildsTheTree();
  void mpdLostEmptiesTheTree();
  void filePathReturnsTheMpdRelativePath();
  void playlistsModelListsTheServerPlaylists();
  void createPlaylistFromDirsSubstitutesTheSlash();
  void createPlaylistFromDirsUniquifiesTheName();
  void renameGoesToTheServer();
  void removeDeletesOnTheServer();
  void mpdLoaderBuildsTracksMarkedAsMpd();
  void mpdLoaderExtractsTheYearFromTheDateTag();

private:
  void awaitTree();
  QStringList topLevelNames() const;

  MpdTest::Server server;
  GuiTest::ConfigDir config;
  SlidingBanner banner;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<DirectoryUi::DirectoryModel::Mpd> tree;
};

void TestMpdModels::initTestCase() {
  if (!MpdTest::Server::installed()) {
    QSKIP("mpd is not installed");
  }
  MpdTest::registerMetaTypes();
  QVERIFY2(server.start(), qPrintable(server.failReason()));

  QVERIFY(config.init({server.url().toString()}));
  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);

  QSignalSpy ready(modus.get(), &ModusOperandi::mpdReady);
  modus->mpd_client.openConnection(server.url());
  QVERIFY(modus->mpd_client.ping());
  QTRY_VERIFY(ready.count() >= 1);

  tree = std::make_unique<DirectoryUi::DirectoryModel::Mpd>(modus->mpd_client);
  // The tree fills itself from mpdReady, which DirectoryModel::Proxy normally wires.
  connect(modus.get(), &ModusOperandi::mpdReady, tree.get(), &DirectoryUi::DirectoryModel::Mpd::onMpdReady);
  connect(modus.get(), &ModusOperandi::mpdLost, tree.get(), &DirectoryUi::DirectoryModel::Mpd::onMpdLost);
  awaitTree();
}

void TestMpdModels::cleanupTestCase() {
  // The models run blocking calls into the client from pooled threads; a task still in flight would deadlock against the client's own thread teardown.
  QThreadPool::globalInstance()->waitForDone();
  tree.reset();
  modus.reset();
  local.reset();
  global.reset();
}

void TestMpdModels::cleanup() {
  QThreadPool::globalInstance()->waitForDone();
  server.resetState();
  QTest::qWait(50);
}

void TestMpdModels::awaitTree() {
  tree->onMpdReady();
  QTRY_VERIFY(tree->rowCount(tree->rootIndex()) > 0);
}

QStringList TestMpdModels::topLevelNames() const {
  QStringList names;
  const QModelIndex root = tree->rootIndex();
  for (int i = 0; i < tree->rowCount(root); i++) {
    names << tree->index(i, 0, root).data().toString();
  }
  return names;
}

void TestMpdModels::modusSwitchesToMpdForAnMpdLibraryPath() {
  QCOMPARE(modus->get(), ModusOperandi::MODUS_MPD);
}

void TestMpdModels::mpdReadyIsEmittedWhenTheConnectionOpens() {
  QSignalSpy ready(modus.get(), &ModusOperandi::mpdReady);

  modus->mpd_client.openConnection(server.url());
  QVERIFY(modus->mpd_client.ping());

  QTRY_VERIFY(ready.count() >= 1);
  QCOMPARE(ready.first().at(0).toUrl(), server.url());
}

void TestMpdModels::directoryTreeMirrorsTheServerRoot() {
  awaitTree();

  const QStringList names = topLevelNames();
  QCOMPARE(names.size(), 4);
  for (const QString &expected : {"wav", "tagged", "covered", "embedded"}) {
    QVERIFY2(names.contains(expected), qPrintable(names.join(", ")));
  }
}

void TestMpdModels::directoryTreeFetchesChildrenOnDemand() {
  awaitTree();

  QModelIndex tagged;
  const QModelIndex root = tree->rootIndex();
  for (int i = 0; i < tree->rowCount(root); i++) {
    if (tree->index(i, 0, root).data().toString() == "tagged") {
      tagged = tree->index(i, 0, root);
    }
  }
  QVERIFY(tagged.isValid());
  QVERIFY(tree->hasChildren(tagged));

  if (tree->canFetchMore(tagged)) {
    tree->fetchMore(tagged);
  }

  QTRY_COMPARE(tree->rowCount(tagged), 2);
  QStringList children;
  for (int i = 0; i < tree->rowCount(tagged); i++) {
    children << tree->index(i, 0, tagged).data().toString();
  }
  QVERIFY(children.contains("one.mp3"));
  QVERIFY(children.contains("two.flac"));
}

void TestMpdModels::directoryTreeFiltersTopLevelEntries() {
  awaitTree();

  tree->filter("tag");
  QTRY_COMPARE(tree->rowCount(tree->rootIndex()), 1);
  QCOMPARE(topLevelNames(), QStringList({"tagged"}));

  tree->filter("");
  QTRY_COMPARE(tree->rowCount(tree->rootIndex()), 4);
}

void TestMpdModels::directoryTreeSortsDirectoriesFirst() {
  awaitTree();
  tree->sort(0, Qt::AscendingOrder);

  // Every top level entry here is a directory, so sorting is alphabetical.
  QStringList sorted = topLevelNames();
  QStringList expected = sorted;
  expected.sort(Qt::CaseInsensitive);
  QCOMPARE(sorted, expected);
}

void TestMpdModels::databaseUpdateRebuildsTheTree() {
  awaitTree();
  QVERIFY(QDir().mkpath(server.musicDir() + "/extra"));
  QVERIFY(MpdTest::writeWav(server.musicDir() + "/extra/added.wav", 1));

  modus->mpd_client.updateDb();

  QTRY_VERIFY_WITH_TIMEOUT(topLevelNames().contains("extra"), 30000);

  QVERIFY(QFile::remove(server.musicDir() + "/extra/added.wav"));
  QVERIFY(QDir(server.musicDir() + "/extra").removeRecursively());
  modus->mpd_client.updateDb();
  QTRY_VERIFY_WITH_TIMEOUT(!topLevelNames().contains("extra"), 30000);
}

void TestMpdModels::mpdLostEmptiesTheTree() {
  awaitTree();
  QVERIFY(tree->rowCount(tree->rootIndex()) > 0);

  tree->onMpdLost();

  QCOMPARE(tree->rowCount(tree->rootIndex()), 0);

  tree->onMpdReady();
  QTRY_COMPARE(tree->rowCount(tree->rootIndex()), 4);
}

void TestMpdModels::filePathReturnsTheMpdRelativePath() {
  awaitTree();

  QModelIndex tagged;
  const QModelIndex root = tree->rootIndex();
  for (int i = 0; i < tree->rowCount(root); i++) {
    if (tree->index(i, 0, root).data().toString() == "tagged") {
      tagged = tree->index(i, 0, root);
    }
  }
  QVERIFY(tagged.isValid());

  QCOMPARE(tree->filePath(tagged), QString("tagged"));
}

void TestMpdModels::playlistsModelListsTheServerPlaylists() {
  QVERIFY(modus->mpd_client.createPlaylist({"tagged/one.mp3"}, "alpha"));
  QVERIFY(modus->mpd_client.createPlaylist({"tagged/two.flac"}, "beta"));

  PlaylistsUi::Mpd::Model model(*local, modus->mpd_client);
  model.loadAsync();

  QTRY_COMPARE(model.rowCount(QModelIndex()), 2);
  QStringList names;
  for (int i = 0; i < model.rowCount(QModelIndex()); i++) {
    names << model.index(i, 0, QModelIndex()).data().toString();
  }
  QVERIFY(names.contains("alpha"));
  QVERIFY(names.contains("beta"));
}

void TestMpdModels::createPlaylistFromDirsSubstitutesTheSlash() {
  PlaylistsUi::Mpd::Model model(*local, modus->mpd_client);
  model.loadAsync();
  QTRY_COMPARE(model.rowCount(QModelIndex()), 0);

  auto future = model.createPlaylistAsync({QDir("tagged")}, "");
  future.waitForFinished();

  // mpd playlist names cannot nest, so '/' becomes U+2215.
  QTRY_COMPARE(model.rowCount(QModelIndex()), 1);
  const QString name = model.index(0, 0, QModelIndex()).data().toString();
  QVERIFY2(!name.contains('/'), qPrintable(name));
  QCOMPARE(name, QString("tagged"));
}

void TestMpdModels::createPlaylistFromDirsUniquifiesTheName() {
  QVERIFY(modus->mpd_client.createPlaylist({"tagged/one.mp3"}, "tagged"));

  PlaylistsUi::Mpd::Model model(*local, modus->mpd_client);
  model.loadAsync();
  QTRY_COMPARE(model.rowCount(QModelIndex()), 1);

  auto future = model.createPlaylistAsync({QDir("tagged")}, "");
  future.waitForFinished();

  QTRY_COMPARE(model.rowCount(QModelIndex()), 2);
  QStringList names;
  for (int i = 0; i < model.rowCount(QModelIndex()); i++) {
    names << model.index(i, 0, QModelIndex()).data().toString();
  }
  QCOMPARE(names.filter("tagged").size(), 2);
  QVERIFY2(names.filter("tagged").at(0) != names.filter("tagged").at(1), qPrintable(names.join(", ")));
}

void TestMpdModels::renameGoesToTheServer() {
  QVERIFY(modus->mpd_client.createPlaylist({"tagged/one.mp3"}, "old_name"));

  PlaylistsUi::Mpd::Model model(*local, modus->mpd_client);
  model.loadAsync();
  QTRY_COMPARE(model.rowCount(QModelIndex()), 1);

  model.onRename("old_name", "new_name");

  QStringList names;
  for (const auto &entity : modus->mpd_client.playlists()) {
    names << entity.path();
  }
  QVERIFY2(names.contains("new_name"), qPrintable(names.join(", ")));
  QVERIFY(!names.contains("old_name"));
}

void TestMpdModels::removeDeletesOnTheServer() {
  QVERIFY(modus->mpd_client.createPlaylist({"tagged/one.mp3"}, "to_remove"));

  PlaylistsUi::Mpd::Model model(*local, modus->mpd_client);
  model.loadAsync();
  QTRY_COMPARE(model.rowCount(QModelIndex()), 1);

  model.remove(model.index(0, 0, QModelIndex()));

  QTRY_VERIFY(modus->mpd_client.playlists().isEmpty());
}

void TestMpdModels::mpdLoaderBuildsTracksMarkedAsMpd() {
  QVERIFY(modus->mpd_client.createPlaylist({"tagged/one.mp3", "tagged/two.flac"}, "loaded"));

  Playlist::MpdLoader loader(modus->mpd_client);
  const auto tracks = loader.playlistTracks("loaded");

  QCOMPARE(tracks.size(), 2);
  for (const auto &t : tracks) {
    QVERIFY(t.isMpd());
    QCOMPARE(t.album(), QString("Tagged Album"));
    QCOMPARE(t.artist(), QString("Tagged Artist"));
  }
  QCOMPARE(tracks.at(0).path(), QString("tagged/one.mp3"));
  QCOMPARE(tracks.at(0).title(), QString("One"));
}

void TestMpdModels::mpdLoaderExtractsTheYearFromTheDateTag() {
  Playlist::MpdLoader loader(modus->mpd_client);

  const auto tracks = loader.dirsTracks({QDir("tagged")});

  QCOMPARE(tracks.size(), 2);
  QCOMPARE(tracks.at(0).year(), 1999);
}

QTEST_MAIN(TestMpdModels)

#include "tst_mpdmodels.moc"
