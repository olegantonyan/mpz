#include "fixture.h"

#include "backgroundtasks.h"
#include "modusoperandi.h"
#include "playlists_ui/playlistscontextmenu.h"
#include "playlists_ui/playlistscontroller.h"
#include "slidingbanner.h"

#include <QLineEdit>
#include <QListView>
#include <QMimeData>

namespace {
  QMimeData *rowMime(int row) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << row;
    auto *mime = new QMimeData;
    mime->setData(QStringLiteral("application/x-mpz-playlist-row"), bytes);
    return mime;
  }
}

class TestPlaylists : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void createsAPlaylistAndSelectsIt();
  void persistsAcrossAControllerRestart();
  void restoredPlaylistIsSelectedInTheView();
  void deleteKeyRemovesAndPersists();
  void removingTheLastPlaylistEmpties();
  void reorderPersistsTheNewOrder();
  void filteringHidesRowsAndDisablesDragging();
  void reloadFromFilesystemRefreshesAndPersists();
  void m3uExportListsTheTrackPaths();

private:
  GuiTest::ConfigDir config;
  SlidingBanner banner;
  QListView view;
  QLineEdit search;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<BackgroundTasks> tasks;
  std::unique_ptr<PlaylistsUi::Controller> playlists;

  void build();
  void create(const QString &name, const QStringList &titles);
  QStringList shownNames() const;
};

void TestPlaylists::initTestCase() {
  QVERIFY(config.init());
}

void TestPlaylists::init() {
  build();
}

void TestPlaylists::cleanup() {
  playlists.reset();
  tasks.reset();
  modus.reset();
  local.reset();
  QFile::remove(config.path() + "/local.yml");
}

void TestPlaylists::build() {
  playlists.reset();
  tasks.reset();
  modus.reset();
  local.reset();

  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  tasks = std::make_unique<BackgroundTasks>();
  playlists = std::make_unique<PlaylistsUi::Controller>(&view, &search, *local, tasks.get(), *modus);

  QSignalSpy loaded(playlists.get(), &PlaylistsUi::Controller::asyncLoadFinished);
  QVERIFY(loaded.wait());
}

void TestPlaylists::create(const QString &name, const QStringList &titles) {
  QSignalSpy created(playlists.get(), &PlaylistsUi::Controller::loaded);
  playlists->on_createPlaylistFromTracks(GuiTest::tracks(titles), name);
  QVERIFY(created.wait());
}

QStringList TestPlaylists::shownNames() const {
  QStringList result;
  for (int i = 0; i < view.model()->rowCount(); i++) {
    result << view.model()->index(i, 0).data().toString();
  }
  return result;
}

void TestPlaylists::createsAPlaylistAndSelectsIt() {
  create("rock", {"one", "two"});

  QCOMPARE(playlists->playlistsCount(), 1);
  QCOMPARE(shownNames(), QStringList({"rock"}));
  QCOMPARE(playlists->currentPlaylist()->name(), QString("rock"));
  QCOMPARE(view.currentIndex().row(), 0);
  QCOMPARE(playlists->playlistByName("rock")->tracks().size(), 2);
}

void TestPlaylists::persistsAcrossAControllerRestart() {
  create("rock", {"one", "two"});
  local->sync();

  build();

  QCOMPARE(playlists->playlistsCount(), 1);
  auto restored = playlists->playlistByName("rock");
  QVERIFY(restored != nullptr);
  // uids are minted afresh on deserialize, so compare by content.
  QCOMPARE(restored->tracks().size(), 2);
  QCOMPARE(restored->tracks().first().title(), QString("one"));
  QCOMPARE(restored->tracks().first().path(), QString("/music/one.mp3"));
}

void TestPlaylists::restoredPlaylistIsSelectedInTheView() {
  create("rock", {"one"});
  local->sync();

  build();

  QCOMPARE(view.currentIndex().row(), 0);
  QVERIFY(!view.selectionModel()->selectedIndexes().isEmpty());
}

void TestPlaylists::deleteKeyRemovesAndPersists() {
  create("rock", {"one"});
  create("jazz", {"two"});
  QCOMPARE(playlists->playlistsCount(), 2);

  view.setCurrentIndex(view.model()->index(0, 0));
  view.selectionModel()->select(view.model()->index(0, 0), QItemSelectionModel::Select);
  QTest::keyClick(&view, Qt::Key_Delete);

  QCOMPARE(playlists->playlistsCount(), 1);
  QCOMPARE(shownNames(), QStringList({"jazz"}));
  QCOMPARE(local->playlists().size(), 1);
}

void TestPlaylists::removingTheLastPlaylistEmpties() {
  create("rock", {"one"});
  QSignalSpy emptied(playlists.get(), &PlaylistsUi::Controller::emptied);

  view.setCurrentIndex(view.model()->index(0, 0));
  view.selectionModel()->select(view.model()->index(0, 0), QItemSelectionModel::Select);
  QTest::keyClick(&view, Qt::Key_Delete);

  QCOMPARE(playlists->playlistsCount(), 0);
  QCOMPARE(emptied.count(), 1);
}

void TestPlaylists::reorderPersistsTheNewOrder() {
  create("one", {"a"});
  create("two", {"b"});
  create("three", {"c"});

  std::unique_ptr<QMimeData> mime(rowMime(0));
  view.model()->dropMimeData(mime.get(), Qt::MoveAction, 3, 0, QModelIndex());

  QCOMPARE(shownNames(), QStringList({"two", "three", "one"}));
  QCOMPARE(local->playlists().first()->name(), QString("two"));
}

void TestPlaylists::filteringHidesRowsAndDisablesDragging() {
  create("rock", {"a"});
  create("jazz", {"b"});

  search.setText("roc");
  QCOMPARE(shownNames(), QStringList({"rock"}));
  QCOMPARE(view.dragDropMode(), QAbstractItemView::NoDragDrop);

  search.clear();
  QCOMPARE(shownNames().size(), 2);
  QCOMPARE(view.dragDropMode(), QAbstractItemView::InternalMove);
}

void TestPlaylists::reloadFromFilesystemRefreshesAndPersists() {
  const QString music = config.path() + "/reload";
  QVERIFY(GuiTest::copyAudioFixtures(music));
  QVector<Track> tracks;
  for (const QString &name : {"silence.mp3", "silence.flac"}) {
    tracks << Track(music + "/" + name);
  }
  QSignalSpy created(playlists.get(), &PlaylistsUi::Controller::loaded);
  playlists->on_createPlaylistFromTracks(tracks, "reloadable");
  QVERIFY(created.wait());

  auto *menu = playlists->findChild<PlaylistsUi::PlaylistsContextMenu *>();
  QVERIFY(menu != nullptr);
  QSignalSpy changed(menu, &PlaylistsUi::PlaylistsContextMenu::playlistChanged);

  // "Reload from filesystem" re-reads the tags of every track in place.
  QVERIFY(QMetaObject::invokeMethod(menu, "on_reload", Q_ARG(QModelIndex, view.model()->index(0, 0))));

  QCOMPARE(changed.count(), 1);
  QCOMPARE(playlists->currentPlaylist()->tracks().size(), 2);
  QCOMPARE(local->playlists().size(), 1);
}

void TestPlaylists::m3uExportListsTheTrackPaths() {
  create("exported", {"one", "two"});

  const QByteArray m3u = playlists->playlistByName("exported")->toM3U();

  QVERIFY(m3u.contains("/music/one.mp3"));
  QVERIFY(m3u.contains("/music/two.mp3"));
}

MPZ_GUI_TEST_MAIN(TestPlaylists)
#include "tst_playlists.moc"
