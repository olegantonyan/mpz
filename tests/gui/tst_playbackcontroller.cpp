#include "fixture.h"

#include "modusoperandi.h"
#include "playback/playbackcontroller.h"
#include "slidingbanner.h"
#include "trayicon.h"
#include "volumecontrol.h"
#ifdef ENABLE_GAPLESS
  #include "replaygain/manager.h"
  #include "replaygain_ui/statusmenu.h"
#endif

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QToolButton>
#include <QWheelEvent>

#include <limits>

// Signal and state plumbing only: nothing here decodes audio.
class TestPlaybackController : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void startsStoppedWithNoTrack();
  void volumeIsClampedAndEchoed();
  void seekbarMaximumFollowsTheTrackKind();
  void seekIsIgnoredWhileStopped();
  void volumeControlClampsAndLabels();
  void volumeControlWheelStepsByFive();
  void trayIconEnablesActionsPerState();
  void trayIconShowsTheElapsedTime();
  void replayGainStatusMenuMirrorsTheMode();

private:
  GuiTest::ConfigDir config;
  SlidingBanner banner;
  QMainWindow window;
  QToolButton stop_button, play_button, pause_button, prev_button, next_button, volume_button;
  Playback::Seekbar seekbar;
  QLabel time_label;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<Playback::Controller> player;

  Playback::Controls controls();
  static Track localTrack(quint64 duration_ms);
  static QAction *actionNamed(TrayIcon &tray, const QString &text);
};

void TestPlaybackController::initTestCase() {
  QVERIFY(config.init());
  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  player = std::make_unique<Playback::Controller>(controls(), 262144, QByteArray(), 100, *modus);
}

void TestPlaybackController::cleanupTestCase() {
  player.reset();
  modus.reset();
  local.reset();
  global.reset();
}

Playback::Controls TestPlaybackController::controls() {
  Playback::Controls c;
  c.stop = &stop_button;
  c.play = &play_button;
  c.pause = &pause_button;
  c.prev = &prev_button;
  c.next = &next_button;
  c.seekbar = &seekbar;
  c.time = &time_label;
  return c;
}

Track TestPlaybackController::localTrack(quint64 duration_ms) {
  return Track("/music/song.mp3", 0, "artist", "album", "title", 1, 2000, duration_ms, 2, 320, 44100);
}

QAction *TestPlaybackController::actionNamed(TrayIcon &tray, const QString &text) {
  for (auto *action : tray.findChildren<QAction *>()) {
    if (action->text() == text) {
      return action;
    }
  }
  return nullptr;
}

void TestPlaybackController::startsStoppedWithNoTrack() {
  QCOMPARE(player->state(), Playback::Controller::Stopped);
  QVERIFY(player->isStopped());
  QVERIFY(!player->currentTrack().isValid());
  QCOMPARE(player->controls().play, &play_button);
}

void TestPlaybackController::volumeIsClampedAndEchoed() {
  QSignalSpy spy(player.get(), &Playback::Controller::volumeChanged);

  player->setVolume(150);
  QCOMPARE(spy.last().first().toInt(), 100);
  QCOMPARE(player->volume(), 100);

  player->setVolume(-5);
  QCOMPARE(spy.last().first().toInt(), 0);
  QCOMPARE(player->volume(), 0);

  player->setVolume(42);
  QCOMPARE(spy.last().first().toInt(), 42);
  QCOMPARE(player->volume(), 42);
}

void TestPlaybackController::seekbarMaximumFollowsTheTrackKind() {
  player->play(localTrack(180000));
  QCOMPARE(seekbar.maximum(), 180);
  player->stop();

  // Unknown length: the bar goes unbounded. Streams take the same branch, but
  // playing one here would hold the test for the stream's 30 s connect timeout.
  player->play(localTrack(0));
  QCOMPARE(seekbar.maximum(), std::numeric_limits<int>::max());
  player->stop();
}

void TestPlaybackController::seekIsIgnoredWhileStopped() {
  player->stop();
  QSignalSpy spy(player.get(), &Playback::Controller::seeked);
  seekbar.setValue(0);

  QTest::mouseClick(&seekbar, Qt::LeftButton, Qt::NoModifier, QPoint(seekbar.width() / 2, 2));

  QCOMPARE(spy.count(), 0);
  QCOMPARE(seekbar.value(), 0);
}

void TestPlaybackController::volumeControlClampsAndLabels() {
  VolumeControl volume(&volume_button, 50);
  QCOMPARE(volume_button.text(), QString("50%"));

  volume.setValue(150);
  QCOMPARE(volume_button.text(), QString("100%"));

  volume.setValue(-10);
  QCOMPARE(volume_button.text(), QString("0%"));
}

void TestPlaybackController::volumeControlWheelStepsByFive() {
  VolumeControl volume(&volume_button, 50);
  QSignalSpy spy(&volume, &VolumeControl::changed);

  QWheelEvent up(QPointF(), QPointF(), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                 Qt::NoScrollPhase, false);
  QCoreApplication::sendEvent(&volume_button, &up);
  QCOMPARE(spy.last().first().toInt(), 55);

  QWheelEvent down(QPointF(), QPointF(), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                   Qt::NoScrollPhase, false);
  QCoreApplication::sendEvent(&volume_button, &down);
  QCOMPARE(spy.last().first().toInt(), 45);
}

void TestPlaybackController::trayIconEnablesActionsPerState() {
  TrayIcon tray(&window);
  auto *play = actionNamed(tray, "Play");
  auto *stop = actionNamed(tray, "Stop");
  QVERIFY(play != nullptr && stop != nullptr);

  tray.on_playerStopped();
  QVERIFY(play->isEnabled());
  QVERIFY(!stop->isEnabled());

  tray.on_playerStarted(localTrack(180000));
  QVERIFY(!play->isEnabled());
  QVERIFY(stop->isEnabled());

  tray.on_playerPaused(localTrack(180000));
  QVERIFY(play->isEnabled());
  QVERIFY(stop->isEnabled());
}

void TestPlaybackController::trayIconShowsTheElapsedTime() {
  TrayIcon tray(&window);
  const Track track = localTrack(180000);

  tray.on_playerStarted(track);
  auto *now_playing = actionNamed(tray, QString("%1 (00:00/03:00)").arg(track.shortText()));
  QVERIFY2(now_playing != nullptr, "now-playing entry missing right after start");

  tray.on_playerProgress(track, 65);
  QCOMPARE(now_playing->text(), QString("%1 (01:05/03:00)").arg(track.shortText()));

  tray.on_playerStopped();
  QVERIFY(now_playing->text().isEmpty());
}

void TestPlaybackController::replayGainStatusMenuMirrorsTheMode() {
#ifdef ENABLE_GAPLESS
  ReplayGain::Manager manager(*global);
  ReplayGainUi::StatusMenu menu(manager, *global);
  QSignalSpy opened(&menu, &ReplayGainUi::StatusMenu::openDialog);

  const auto actions = menu.actions();
  QCOMPARE(actions.size(), 5); // three modes, a separator, and the dialog entry
  QCOMPARE(actions.at(0)->text(), QString("Off"));
  QVERIFY(actions.at(0)->isChecked());

  actions.at(2)->trigger(); // album gain
  QCOMPARE(manager.settings().mode, ReplayGain::Mode::Album);

  actions.last()->trigger();
  QCOMPARE(opened.count(), 1);
  QCOMPARE(manager.settings().mode, ReplayGain::Mode::Album);
#else
  QSKIP("gapless disabled, no replaygain");
#endif
}

MPZ_GUI_TEST_MAIN(TestPlaybackController)
#include "tst_playbackcontroller.moc"
