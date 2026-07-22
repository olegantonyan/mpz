#include "macmenubar.h"

#include "mainwindow.h"
#include "shortcuts.h"
#include "modusoperandi.h"
#include "playback/playbackcontroller.h"
#include "track.h"
#include "sort_ui/sortmenu.h"
#include "config/global.h"
#include "config/local.h"
#include "about_ui/aboutdialog.h"
#include "feedback_ui/feedbackform.h"
#ifdef ENABLE_DEVICES_MENU
  #include "audio_device_ui/devicesmenu.h"
#endif

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QDesktopServices>
#include <QUrl>

MacMenuBar::MacMenuBar(MainWindow *win, Config::Global &global_c, Config::Local &local_c,
                       Shortcuts *shortcuts, Playback::Controller *player, ModusOperandi &modus,
                       SortUi::SortMenu *sort_menu, QAction *cover_toggle, QAction *lyrics_toggle, QAction *lock_toggle) :
  QObject(win), window(win), global_conf(global_c), local_conf(local_c), modus_operandi(modus) {
  auto *bar = window->menuBar();

  auto *app_menu = bar->addMenu(tr("mpz"));

  auto *about = app_menu->addAction(tr("About mpz"));
  about->setMenuRole(QAction::AboutRole);
  connect(about, &QAction::triggered, this, [this]() { AboutDialog(global_conf, local_conf).exec(); });

  auto *prefs = app_menu->addAction(tr("Settings…"));
  prefs->setMenuRole(QAction::PreferencesRole);
  prefs->setShortcut(QKeySequence::Preferences);
  connect(prefs, &QAction::triggered, shortcuts, &Shortcuts::openSettings);

  auto *quit = app_menu->addAction(tr("Quit mpz"));
  quit->setMenuRole(QAction::QuitRole);
  quit->setShortcut(QKeySequence::Quit);
  connect(quit, &QAction::triggered, window, &MainWindow::requestQuit);

  auto *playback = bar->addMenu(tr("Playback"));

  auto *now_playing = playback->addAction("");
  now_playing->setEnabled(false);
  auto *now_playing_sep = playback->addSeparator();

  auto *play_pause = playback->addAction(tr("Play / Pause"));
  play_pause->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::PlayPause));
  connect(play_pause, &QAction::triggered, shortcuts, &Shortcuts::playPause);

  auto *stop_action = playback->addAction(tr("Stop"));
  connect(stop_action, &QAction::triggered, shortcuts, &Shortcuts::stop);

  playback->addSeparator();

  auto *next_action = playback->addAction(tr("Next Track"));
  next_action->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::Next));
  connect(next_action, &QAction::triggered, shortcuts, &Shortcuts::next);

  auto *prev_action = playback->addAction(tr("Previous Track"));
  prev_action->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::Prev));
  connect(prev_action, &QAction::triggered, shortcuts, &Shortcuts::prev);

  auto update_now_playing = [now_playing, now_playing_sep](const Track &track) {
    const QString text = track.shortText();
    const bool visible = !text.isEmpty();
    now_playing->setVisible(visible);
    now_playing_sep->setVisible(visible);
    now_playing->setText(text);
  };
  auto apply_active = [=](const Track &track, bool playing) {
    play_pause->setText(playing ? tr("Pause") : tr("Play"));
    stop_action->setEnabled(true);
    next_action->setEnabled(true);
    prev_action->setEnabled(true);
    update_now_playing(track);
  };
  auto apply_stopped = [=]() {
    play_pause->setText(tr("Play"));
    stop_action->setEnabled(false);
    next_action->setEnabled(false);
    prev_action->setEnabled(false);
    now_playing->setVisible(false);
    now_playing_sep->setVisible(false);
  };
  connect(player, &Playback::Controller::started, this, [=](const Track &t) { apply_active(t, true); });
  connect(player, &Playback::Controller::paused, this, [=](const Track &t) { apply_active(t, false); });
  connect(player, &Playback::Controller::trackChanged, this, update_now_playing);
  connect(player, &Playback::Controller::stopped, this, apply_stopped);
  apply_stopped();

  playback->addSeparator();

  auto *vol_up = playback->addAction(tr("Volume Up"));
  vol_up->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::VolumeUp));
  connect(vol_up, &QAction::triggered, shortcuts, &Shortcuts::volumeUp);

  auto *vol_down = playback->addAction(tr("Volume Down"));
  vol_down->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::VolumeDown));
  connect(vol_down, &QAction::triggered, shortcuts, &Shortcuts::volumeDown);

#ifdef ENABLE_GAPLESS
  playback->addSeparator();
  auto *equalizer = playback->addAction(tr("Equalizer…"));
  equalizer->setEnabled(!global_conf.disableGapless());
  connect(equalizer, &QAction::triggered, shortcuts, &Shortcuts::openEqualizer);
  connect(playback, &QMenu::aboutToShow, this, [this, equalizer]() {
    equalizer->setEnabled(!global_conf.disableGapless());
  });
#endif

#ifdef ENABLE_DEVICES_MENU
  playback->addSeparator();

  auto *output = new AudioDeviceUi::DevicesMenu(window, local_c);
  output->setTitle(tr("Output Device"));
  connect(output, &AudioDeviceUi::DevicesMenu::outputDeviceChanged, player, &Playback::Controller::setOutputDevice);
  playback->addMenu(output);
  output->menuAction()->setEnabled(modus_operandi.get() == ModusOperandi::MODUS_LOCALFS);
  connect(&modus_operandi, &ModusOperandi::changed, this, [=](auto mode) {
    output->menuAction()->setEnabled(mode == ModusOperandi::MODUS_LOCALFS);
  });
#endif

#ifdef ENABLE_MPD_SUPPORT
  playback->addSeparator();

  auto *mpd_update = playback->addAction(tr("mpd update"));
  connect(mpd_update, &QAction::triggered, this, [this]() {
    modus_operandi.mpd_client.updateDb();
  });
  mpd_update->setEnabled(modus_operandi.get() == ModusOperandi::MODUS_MPD);
  connect(&modus_operandi, &ModusOperandi::changed, this, [=](auto mode) {
    mpd_update->setEnabled(mode == ModusOperandi::MODUS_MPD);
  });
#endif

  auto *view = bar->addMenu(tr("View"));
  auto *sort_submenu = view->addMenu(tr("Sort"));
  sort_menu->attachToMenu(sort_submenu);

  view->addSeparator();

  view->addAction(cover_toggle);
  view->addAction(lyrics_toggle);
  view->addAction(lock_toggle);

  view->addSeparator();

  auto *jump = view->addAction(tr("Jump to Playing Track"));
  jump->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::JumpToPlayingTrack));
  connect(jump, &QAction::triggered, shortcuts, &Shortcuts::jumpToPLayingTrack);

  auto *playback_log_action = view->addAction(tr("Playback Log"));
  playback_log_action->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::OpenPlaybackLog));
  connect(playback_log_action, &QAction::triggered, shortcuts, &Shortcuts::openPlabackLog);

  auto *shortcuts_action = view->addAction(tr("Keyboard Shortcuts"));
  shortcuts_action->setShortcut(Shortcuts::sequenceFor(Shortcuts::Action::OpenShortcutsMenu));
  connect(shortcuts_action, &QAction::triggered, shortcuts, &Shortcuts::openShortcutsMenu);

  view->addSeparator();

  auto *fullscreen = view->addAction(tr("Enter Full Screen"));
  fullscreen->setShortcut(QKeySequence(QKeySequence::FullScreen));
  connect(fullscreen, &QAction::triggered, window, [this]() {
    if (window->isFullScreen()) {
      window->showNormal();
    } else {
      window->showFullScreen();
    }
  });
  connect(view, &QMenu::aboutToShow, this, [this, fullscreen]() {
    fullscreen->setText(window->isFullScreen() ? tr("Exit Full Screen") : tr("Enter Full Screen"));
  });

  auto *window_menu = bar->addMenu(tr("Window"));

  auto *minimize = window_menu->addAction(tr("Minimize"));
  minimize->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
  connect(minimize, &QAction::triggered, window, &QWidget::showMinimized);

  auto *zoom = window_menu->addAction(tr("Zoom"));
  connect(zoom, &QAction::triggered, window, [this]() {
    if (window->isMaximized()) {
      window->showNormal();
    } else {
      window->showMaximized();
    }
  });

  window_menu->addSeparator();

  auto *close_window = window_menu->addAction(tr("Close"));
  close_window->setShortcut(QKeySequence::Close);
  connect(close_window, &QAction::triggered, window, [this]() { window->hide(); });

  auto *help = bar->addMenu(tr("Help"));

  auto *website = help->addAction(tr("mpz Website"));
  connect(website, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl("https://mpz-player.org"));
  });

  auto *github = help->addAction(tr("mpz GitHub"));
  connect(github, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl("https://github.com/olegantonyan/mpz"));
  });

  auto *feedback = help->addAction(tr("Send Feedback…"));
  connect(feedback, &QAction::triggered, this, [this]() { FeedbackForm(local_conf).exec(); });

  auto *bug_report = help->addAction(tr("Report a Bug…"));
  connect(bug_report, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl("https://github.com/olegantonyan/mpz/issues"));
  });
}
