#include <QtTest>
#include <QLineEdit>
#include <QListView>
#include <QTemporaryDir>

#include "backgroundtasks.h"
#include "config/global.h"
#include "config/local.h"
#include "modusoperandi.h"
#include "playback/dispatch.h"
#include "playlists_ui/playlistscontroller.h"
#include "slidingbanner.h"

namespace {
  Track mk(const QString &title) {
    return Track("/music/" + title + ".mp3", 0, "artist", "album", title, 1, 2000, 1000, 2, 320, 44100);
  }

  QString playedTitle(const QSignalSpy &spy, int at = 0) {
    return spy.at(at).first().value<Track>().title();
  }
}

class TestDispatch : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void startRequested_playsTheSelectedTrack();
  void startRequested_fallsBackToTheCurrentPlaylist();
  void startRequested_emitsNoTrackToStartWhenEmpty();
  void next_walksSequentiallyAndWrapsAround();
  void next_stopsAtTheEndInNoLoopMode();
  void next_honoursThePerPlaylistNoLoopOverride();
  void next_stopsWhenNothingIsPlaying();
  void prev_wrapsToTheLastTrack();
  void randomMode_isEnabledByThePlaylistOverride();
  void aboutToFinish_prearmsTheNextTrack();
  void aboutToFinish_clearsWhenNoLoopEnds();
  void playlistContentChanged_dropsAStalePreparedPick();
  void playlistContentChanged_stopsWhenThePlayingTrackIsGone();
  void followCursor_jumpsToTheSelectionOnce();

private:
  QTemporaryDir dir;
  SlidingBanner banner;
  QListView view;
  QLineEdit search;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<BackgroundTasks> tasks;
  std::unique_ptr<PlaylistsUi::Controller> playlists;
  std::unique_ptr<Playback::Dispatch> dispatch;
  std::shared_ptr<Playlist::Playlist> playlist;

  void loadPlaylist(const QStringList &titles);
  void startWith(int index);
};

void TestDispatch::initTestCase() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestDispatch::cleanupTestCase() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestDispatch::init() {
  dispatch.reset();
  playlists.reset();
  tasks.reset();
  modus.reset();
  local.reset();
  global.reset();
  QFile::remove(dir.filePath("local.yml"));
  QFile::remove(dir.filePath("global.yml"));

  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  tasks = std::make_unique<BackgroundTasks>();
  playlists = std::make_unique<PlaylistsUi::Controller>(&view, &search, *local, tasks.get(), *modus);
  QSignalSpy loaded(playlists.get(), &PlaylistsUi::Controller::asyncLoadFinished);
  QVERIFY(loaded.wait());

  dispatch = std::make_unique<Playback::Dispatch>(*global, playlists.get());
}

void TestDispatch::loadPlaylist(const QStringList &titles) {
  QVector<Track> tracks;
  for (const auto &t : titles) {
    tracks << mk(t);
  }
  QSignalSpy created(playlists.get(), &PlaylistsUi::Controller::loaded);
  playlists->on_createPlaylistFromTracks(tracks, "test");
  QVERIFY(created.wait());
  playlist = playlists->currentPlaylist();
  QVERIFY(playlist != nullptr);
}

void TestDispatch::startWith(int index) {
  const Track &t = playlist->tracks().at(index);
  dispatch->state().setSelected(t.uid());
  dispatch->on_started(t);
}

void TestDispatch::startRequested_playsTheSelectedTrack() {
  loadPlaylist({"one", "two", "three"});
  dispatch->state().setSelected(playlist->tracks().at(1).uid());
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_startRequested();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(playedTitle(spy), QString("two"));
}

void TestDispatch::startRequested_fallsBackToTheCurrentPlaylist() {
  loadPlaylist({"one", "two"});
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_startRequested();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(playedTitle(spy), QString("one"));
}

void TestDispatch::startRequested_emitsNoTrackToStartWhenEmpty() {
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::noTrackToStart);

  dispatch->on_startRequested();

  QCOMPARE(spy.count(), 1);
}

void TestDispatch::next_walksSequentiallyAndWrapsAround() {
  loadPlaylist({"one", "two", "three"});
  startWith(0);
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_nextRequested();
  QCOMPARE(playedTitle(spy), QString("two"));

  dispatch->on_started(playlist->tracks().at(2));
  dispatch->on_nextRequested();
  QCOMPARE(playedTitle(spy, 1), QString("one"));
}

void TestDispatch::next_stopsAtTheEndInNoLoopMode() {
  global->savePlaybackOrder("sequential (no loop)");
  loadPlaylist({"one", "two"});
  startWith(1);
  QSignalSpy played(dispatch.get(), &Playback::Dispatch::play);
  QSignalSpy stopped(dispatch.get(), &Playback::Dispatch::stop);

  dispatch->on_nextRequested();

  QCOMPARE(played.count(), 0);
  QCOMPARE(stopped.count(), 1);
}

void TestDispatch::next_honoursThePerPlaylistNoLoopOverride() {
  loadPlaylist({"one", "two"});
  playlist->setRandom(Playlist::Playlist::SequentialNoLoop);
  startWith(1);
  QSignalSpy stopped(dispatch.get(), &Playback::Dispatch::stop);

  dispatch->on_nextRequested();

  QCOMPARE(stopped.count(), 1);
}

void TestDispatch::next_stopsWhenNothingIsPlaying() {
  loadPlaylist({"one"});
  QSignalSpy stopped(dispatch.get(), &Playback::Dispatch::stop);

  dispatch->on_nextRequested();

  QCOMPARE(stopped.count(), 1);
}

void TestDispatch::prev_wrapsToTheLastTrack() {
  loadPlaylist({"one", "two", "three"});
  startWith(0);
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_prevRequested();

  QCOMPARE(playedTitle(spy), QString("three"));
}

void TestDispatch::randomMode_isEnabledByThePlaylistOverride() {
  loadPlaylist({"one", "two", "three", "four"});
  playlist->setRandom(Playlist::Playlist::Random);
  startWith(0);
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_nextRequested();

  QCOMPARE(spy.count(), 1);
  // Random never repeats the current track when the playlist has more than one.
  QVERIFY(playedTitle(spy) != QString("one"));
}

void TestDispatch::aboutToFinish_prearmsTheNextTrack() {
  loadPlaylist({"one", "two", "three"});
  startWith(0);
  QSignalSpy prepared(dispatch.get(), &Playback::Dispatch::prepareNext);
  QSignalSpy played(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_aboutToFinish();
  QCOMPARE(prepared.count(), 1);
  QCOMPARE(prepared.first().first().value<Track>().title(), QString("two"));

  dispatch->on_nextRequested();
  QCOMPARE(playedTitle(played), QString("two"));
}

void TestDispatch::aboutToFinish_clearsWhenNoLoopEnds() {
  global->savePlaybackOrder("sequential (no loop)");
  loadPlaylist({"one", "two"});
  startWith(1);
  QSignalSpy prepared(dispatch.get(), &Playback::Dispatch::prepareNext);

  dispatch->on_aboutToFinish();

  QCOMPARE(prepared.count(), 1);
  QVERIFY(!prepared.first().first().value<Track>().isValid());
}

void TestDispatch::playlistContentChanged_dropsAStalePreparedPick() {
  loadPlaylist({"one", "two", "three"});
  startWith(0);
  dispatch->on_aboutToFinish();

  playlist->removeTrack(1);
  QSignalSpy prepared(dispatch.get(), &Playback::Dispatch::prepareNext);

  dispatch->on_playlistContentChanged();

  // Cleared, then re-armed with a pick that still exists.
  QCOMPARE(prepared.count(), 2);
  QVERIFY(!prepared.at(0).first().value<Track>().isValid());
  QCOMPARE(prepared.at(1).first().value<Track>().title(), QString("three"));
}

void TestDispatch::playlistContentChanged_stopsWhenThePlayingTrackIsGone() {
  global->savePlaybackOrder("sequential");
  loadPlaylist({"one", "two"});
  startWith(0);
  global->saveStopWhenTrackRemoved(true);
  playlist->removeTrack(0);

  QSignalSpy stopped(dispatch.get(), &Playback::Dispatch::stop);
  QSignalSpy unloaded(dispatch.get(), &Playback::Dispatch::unloadPlaylistView);

  dispatch->on_playlistContentChanged();

  QCOMPARE(stopped.count(), 1);
  QCOMPARE(unloaded.count(), 1);
}

void TestDispatch::followCursor_jumpsToTheSelectionOnce() {
  global->savePlaybackFollowCursor(true);
  loadPlaylist({"one", "two", "three"});
  startWith(0);
  dispatch->state().setSelected(playlist->tracks().at(2).uid());
  dispatch->state().resetFolowedCursor();
  QSignalSpy spy(dispatch.get(), &Playback::Dispatch::play);

  dispatch->on_nextRequested();

  QCOMPARE(playedTitle(spy), QString("three"));
}

QTEST_MAIN(TestDispatch)
#include "tst_dispatch.moc"
