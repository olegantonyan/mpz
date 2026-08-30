#include "fixture.h"

#include "backgroundtasks.h"
#include "modusoperandi.h"
#include "playlist_ui/playlistcontroller.h"
#include "slidingbanner.h"

#include <QLineEdit>
#include <QMimeData>
#include <QTableView>

namespace {
  QMimeData *rowsMime(const QList<int> &rows) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << static_cast<qint32>(rows.size());
    for (int r : rows) {
      stream << static_cast<qint32>(r);
    }
    auto *mime = new QMimeData;
    mime->setData(QStringLiteral("application/x-mpz-playlist-tracks"), bytes);
    return mime;
  }
}

class TestPlaylistTracks : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void appendTracksFillsTheView();
  void appendFromDiskScansTheDirectory();
  void rearrangeMovesOneRowAndReportsTheChange();
  void rearrangeMovesAContiguousBlock();
  void deleteKeyRemovesTheSelectionAndRestoresTheCursor();
  void filterMatchesArtistAlbumTitleAndFilename();
  void sortByReordersThePlaylist();
  void unloadClearsTheView();

private:
  GuiTest::ConfigDir config;
  SlidingBanner banner;
  QTableView view;
  QLineEdit search;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<BackgroundTasks> tasks;
  std::unique_ptr<PlaylistUi::Controller> controller;
  std::shared_ptr<Playlist::Playlist> playlist;

  void load(const QStringList &titles);
  QStringList shownTitles() const;
};

void TestPlaylistTracks::initTestCase() {
  QVERIFY(config.init());
}

void TestPlaylistTracks::init() {
  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  tasks = std::make_unique<BackgroundTasks>();
  controller = std::make_unique<PlaylistUi::Controller>(&view, &search, tasks.get(), *local, *global, *modus);
  playlist = std::make_shared<Playlist::Playlist>();
  playlist->rename("test");
}

void TestPlaylistTracks::cleanup() {
  controller.reset();
  tasks.reset();
  modus.reset();
  local.reset();
  global.reset();
}

void TestPlaylistTracks::load(const QStringList &titles) {
  playlist->load(GuiTest::tracks(titles));
  controller->on_load(playlist);
}

QStringList TestPlaylistTracks::shownTitles() const {
  QStringList result;
  for (int i = 0; i < view.model()->rowCount(); i++) {
    // Column 0 is the play-state gutter; title lives in the configured columns.
    result << view.model()->index(i, 3).data().toString();
  }
  return result;
}

void TestPlaylistTracks::appendTracksFillsTheView() {
  controller->on_load(playlist);
  QSignalSpy changed(controller.get(), &PlaylistUi::Controller::changed);

  controller->on_appendTracks(GuiTest::tracks({"one", "two"}));

  QCOMPARE(view.model()->rowCount(), 2);
  QCOMPARE(shownTitles(), QStringList({"one", "two"}));
  QCOMPARE(changed.count(), 1);
  QCOMPARE(controller->currentTracks().size(), 2);
}

void TestPlaylistTracks::appendFromDiskScansTheDirectory() {
  const QString music = config.path() + "/append";
  QVERIFY(GuiTest::copyAudioFixtures(music));
  controller->on_load(playlist);
  QSignalSpy changed(controller.get(), &PlaylistUi::Controller::changed);

  controller->on_appendToPlaylist({QDir(music)});

  QVERIFY(changed.wait());
  QCOMPARE(view.model()->rowCount(), 3);
}

void TestPlaylistTracks::rearrangeMovesOneRowAndReportsTheChange() {
  load({"one", "two", "three"});
  QSignalSpy changed(controller.get(), &PlaylistUi::Controller::changed);
  std::unique_ptr<QMimeData> mime(rowsMime({0}));

  view.model()->dropMimeData(mime.get(), Qt::MoveAction, 3, 0, QModelIndex());

  QCOMPARE(shownTitles(), QStringList({"two", "three", "one"}));
  QCOMPARE(playlist->tracks().first().title(), QString("two"));
  // The controller coalesces rowsMoved into one changed() on the next event turn.
  QCOMPARE(changed.count(), 0);
  QVERIFY(changed.wait());
  QCOMPARE(changed.count(), 1);
}

void TestPlaylistTracks::rearrangeMovesAContiguousBlock() {
  load({"one", "two", "three", "four"});
  std::unique_ptr<QMimeData> mime(rowsMime({2, 3}));

  view.model()->dropMimeData(mime.get(), Qt::MoveAction, 0, 0, QModelIndex());

  QCOMPARE(shownTitles(), QStringList({"three", "four", "one", "two"}));
}

void TestPlaylistTracks::deleteKeyRemovesTheSelectionAndRestoresTheCursor() {
  load({"one", "two", "three"});
  view.selectRow(0);
  QSignalSpy changed(controller.get(), &PlaylistUi::Controller::changed);

  QTest::keyClick(&view, Qt::Key_Delete);

  QCOMPARE(shownTitles(), QStringList({"two", "three"}));
  QCOMPARE(playlist->tracks().size(), 2);
  QCOMPARE(changed.count(), 1);
  QCOMPARE(view.currentIndex().row(), 0);
}

void TestPlaylistTracks::filterMatchesArtistAlbumTitleAndFilename() {
  playlist->load({GuiTest::track("Dogs", "Animals", "Pink Floyd"),
                  GuiTest::track("Schism", "Lateralus", "Tool")});
  controller->on_load(playlist);

  search.setText("Tool");
  QCOMPARE(shownTitles(), QStringList({"Schism"}));

  search.setText("animals");
  QCOMPARE(shownTitles(), QStringList({"Dogs"}));

  search.setText("Dogs.mp3");
  QCOMPARE(shownTitles(), QStringList({"Dogs"}));

  search.clear();
  QCOMPARE(view.model()->rowCount(), 2);
}

void TestPlaylistTracks::sortByReordersThePlaylist() {
  load({"zzz", "aaa", "mmm"});

  controller->sortBy("Title");
  QCOMPARE(shownTitles(), QStringList({"aaa", "mmm", "zzz"}));

  controller->sortBy("-Title");
  QCOMPARE(shownTitles(), QStringList({"zzz", "mmm", "aaa"}));
}

void TestPlaylistTracks::unloadClearsTheView() {
  load({"one", "two"});

  controller->on_unload();

  QCOMPARE(view.model()->rowCount(), 0);
  QVERIFY(controller->currentTracks().isEmpty());
}

MPZ_GUI_TEST_MAIN(TestPlaylistTracks)
#include "tst_playlisttracks.moc"
