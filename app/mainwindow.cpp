#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "shortcuts_ui/shortcutsdialog.h"
#include "coverart/covers.h"
#include "coverart/coverartwidget.h"
#include "lyrics/lyricswidget.h"
#include "icons.h"
#include "mpzapplication.h"

#include <QDebug>
#include <QApplication>
#include <QStyle>
#include <QEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHash>
#include <QTimer>
#include <QDockWidget>
#include <QFontDatabase>

#include "settings_ui/settingsdialog.h"

#if defined(ENABLE_UPDATE_CHECK)
  #include "update_check/updatechecker.h"
#endif

MainWindow::MainWindow(const QStringList &args, IPC::Instance *instance, Config::Local &local_c, Config::Global &global_c, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  local_conf(local_c),
  global_conf(global_c),
  trayicon(nullptr),
  banner(new SlidingBanner(this)),
  modus_operandi(local_c, banner)
  {
#if defined(MPRIS_ENABLE)
  mpris = nullptr;
#endif
  ui->setupUi(this);
  ui->verticalLayout->insertWidget(1, banner);
  QIcon app_icon;
  for (const QString &size : {"16x16", "22x22", "24x24", "32x32", "48x48", "64x64", "256x256"})
    app_icon.addFile(":/app/resources/icons/" + size + "/mpz.png");
  setWindowIcon(app_icon);

  spinner = new BusySpinner(ui->widgetSpinner, this);

  library = new DirectoryUi::Controller(ui->treeView, ui->treeViewSearch, ui->comboBoxLibraries, ui->toolButtonLibraries, ui->toolButtonLibrarySort, local_conf, global_conf, modus_operandi, this);
  playlists = new PlaylistsUi::Controller(ui->listView, ui->listViewSearch, local_conf, spinner, modus_operandi, this);
  playlist = new PlaylistUi::Controller(ui->tableView, ui->tableViewSearch, spinner, local_conf, global_conf, modus_operandi, this);

  ui->toolButtonLibrarySort->setIcon(Icons::get(Icons::Icon::Sort));
  ui->toolButtonLibraries->setIcon(Icons::get(Icons::Icon::Settings));
  ui->toolButtonLibraries->setToolButtonStyle(Qt::ToolButtonIconOnly);

  ui->stopButton->setIcon(Icons::get(Icons::Icon::Stop));
  ui->playButton->setIcon(Icons::get(Icons::Icon::Play));
  ui->pauseButton->setIcon(Icons::get(Icons::Icon::Pause));
  ui->nextButton->setIcon(Icons::get(Icons::Icon::Next));
  ui->prevButton->setIcon(Icons::get(Icons::Icon::Prev));
  Playback::Controls pc;
  pc.next = ui->nextButton;
  pc.prev = ui->prevButton;
  pc.stop = ui->stopButton;
  pc.play = ui->playButton;
  pc.pause = ui->pauseButton;
  pc.seekbar = ui->progressBar;
  pc.time = ui->timeLabel;
  ui->timeLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  player = new Playback::Controller(pc, streamBuffer(), local_conf.outputDeviceId(), modus_operandi, this);
  if (local_conf.volume() > 0) {
    player->setVolume(local_conf.volume());
  }

  connect(library, &DirectoryUi::Controller::createNewPlaylist, playlists, &PlaylistsUi::Controller::on_createPlaylist);
  connect(library, &DirectoryUi::Controller::appendToCurrentPlaylist, playlist, &PlaylistUi::Controller::on_appendToPlaylist);
  connect(playlist, &PlaylistUi::Controller::createPlaylistRequested, playlists, &PlaylistsUi::Controller::on_createPlaylist);
  connect(playlists, &PlaylistsUi::Controller::selected, playlist, &PlaylistUi::Controller::on_load);
  connect(playlists, &PlaylistsUi::Controller::emptied, playlist, &PlaylistUi::Controller::on_unload);
  connect(playlist, &PlaylistUi::Controller::activated, player, &Playback::Controller::play);
  connect(player, &Playback::Controller::started, playlist, &PlaylistUi::Controller::on_start);
  connect(player, &Playback::Controller::paused, playlist, &PlaylistUi::Controller::on_pause);
  connect(player, &Playback::Controller::stopped, playlist, &PlaylistUi::Controller::on_stop);
  connect(playlist, &PlaylistUi::Controller::changed, playlists, &PlaylistsUi::Controller::on_playlistChanged);
  connect(player, &Playback::Controller::started, playlists, &PlaylistsUi::Controller::on_start);
  connect(player, &Playback::Controller::stopped, playlists, &PlaylistsUi::Controller::on_stop);

  setupDockWidgets();
  setupUiSettings();

  setupPlaybackDispatch();
  setupStatusBar();
  setupTrayIcon();
  setupOrderCombobox();
  setupPerPlaylistOrderCombobox();
  setupFollowCursorCheckbox();
  setupVolumeControl();
  setupMainMenu();
  setupShortcuts();
  setupSortMenu();
#ifdef Q_OS_MACOS
  setupMacMenuBar();
  setupMacMediaControls();
  setupMacDockMenu();
#endif
#ifdef SMTC_ENABLE
  setupWindowsMediaControls();
#endif
#ifdef Q_OS_WIN
  setupWindowsTaskbar();
#endif
#if defined(MPRIS_ENABLE)
  setupMpris();
#endif
  setupWindowTitle();
  setupPlaybackLog();
  setupSleepLock();
  setupOutputDevice();

  preloadPlaylist(args);

  connect(instance, &IPC::Instance::load_files_received, this, [=](const QStringList &lst) {
    preloadPlaylist(lst);
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
  });

#ifdef ENABLE_MPD_SUPPORT
  setupMpdOrder();
#endif
  CoverArt::Covers::instance(modus_operandi);

#if defined(ENABLE_UPDATE_CHECK)
  setupUpdateChecker();
#endif
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::toggleHidden() {
  if (isHidden()) {
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
  } else {
    hide();
  }
}

int MainWindow::streamBuffer() {
  int stream_buffer = global_conf.streamBufferSize();
  if (stream_buffer == 0) {
    stream_buffer = 131072;
    global_conf.saveStreamBufferSize(stream_buffer);
  }
  return stream_buffer;
}

void MainWindow::setupUiSettings() {
  restoreGeometry(local_conf.windowGeomentry());
  restoreState(local_conf.windowState());

  connect(ui->splitter, &QSplitter::splitterMoved, this, [=](int pos, int index) {
    Q_UNUSED(pos)
    Q_UNUSED(index)
    QList<int> list;
    list.append(ui->splitter->sizes().at(0));
    list.append(ui->splitter->sizes().at(1));
    list.append(ui->splitter->sizes().at(2));
    local_conf.saveSplitterSizes(list);
  });

  auto splitter_sizes = local_conf.splitterSizes();
  if (splitter_sizes.size() >= 3) {
    ui->splitter->setSizes(splitter_sizes);
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
#ifdef Q_OS_MACOS
  // Native Mac idiom for a media player: the red button / Cmd-W hides the
  // window and playback keeps going; only Cmd-Q (requestQuit) really quits.
  if (!quitting) {
    hide();
    event->ignore();
    return;
  }
#else
  if (!quitting && trayicon != nullptr && global_conf.trayIconEnabled() && global_conf.minimizeToTray()) {
    hide();
    event->ignore();
    return;
  }
#endif
  if (modus_operandi.get() != ModusOperandi::MODUS_MPD || global_conf.mpdStopPlayerOnClose()) {
    player->stop();
  }
  local_conf.saveWindowGeometry(saveGeometry());
  local_conf.saveWindowState(saveState());
  local_conf.sync();
  global_conf.sync();
  if (trayicon != nullptr) {
    trayicon->hide();
  }
  QMainWindow::closeEvent(event);
  qApp->closeAllWindows();
}

void MainWindow::requestQuit() {
  quitting = true;
  close();
#ifdef Q_OS_MACOS
  // quitOnLastWindowClosed is off on macOS (see main.cpp) so closing windows
  // no longer ends the event loop — quit explicitly.
  qApp->quit();
#endif
}

#ifdef Q_OS_MACOS
void MainWindow::onAppActivated() {
  // Dock click while window is hidden: restore it. Also fires on Cmd-Tab — harmless because the window is then already visible.
  if (isHidden()) {
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
  }
}
#endif

void MainWindow::setupOrderCombobox() {
  ui->orderComboBox->addItem(tr("sequential"));
  ui->orderComboBox->addItem(tr("random"));
  ui->orderComboBox->addItem(tr("sequential (no loop)"));

  QHash<QString, int> combobox_item_position;
  combobox_item_position.insert("sequential", 0);
  combobox_item_position.insert("random", 1);
  combobox_item_position.insert("sequential (no loop)", 2);

  ui->orderComboBox->setCurrentIndex(combobox_item_position.value(global_conf.playbackOrder(), 0));

  connect(ui->orderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int idx) {
    QString order_name = combobox_item_position.key(idx, "sequential");
    global_conf.savePlaybackOrder(order_name);
    global_conf.sync();
#if defined(MPRIS_ENABLE)
    if (mpris) {
      mpris->on_shuffleChanged(order_name == "random");
    }
#endif
#ifdef ENABLE_MPD_SUPPORT
    mpd_order->onOrderChanged();
#endif
  });
#if defined(MPRIS_ENABLE)
  if (mpris) {
    mpris->on_shuffleChanged(global_conf.playbackOrder() == "random");
  }
#endif
}

void MainWindow::setupPerPlaylistOrderCombobox() {
  ui->perPlaylistOrdercomboBox->addItem(tr("(use global)"));
  ui->perPlaylistOrdercomboBox->addItem(tr("random"));
  ui->perPlaylistOrdercomboBox->addItem(tr("sequential"));
  ui->perPlaylistOrdercomboBox->addItem(tr("sequential (no loop)"));
  connect(playlists, &PlaylistsUi::Controller::selected, this, [=](const std::shared_ptr<Playlist::Playlist> playlist) {
    if (playlist != nullptr) {
      if (playlist->random() == Playlist::Playlist::Random) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(1);
      } else if (playlist->random() == Playlist::Playlist::Sequential) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(2);
      } else if (playlist->random() == Playlist::Playlist::SequentialNoLoop) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(3);
      } else if (playlist->random() == Playlist::Playlist::None) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(0);
      }
    }
  });
  connect(ui->perPlaylistOrdercomboBox, QOverload<int>::of(&QComboBox::activated), this, [=](int idx) {
    auto current_playlist = playlists->playlistByTrackUid(dispatch->state().selectedTrack());
    if (current_playlist != nullptr) {
      current_playlist->setRandom(static_cast<Playlist::Playlist::PlaylistRandom>(idx));
      playlists->on_playlistChanged(current_playlist);
#ifdef ENABLE_MPD_SUPPORT
      mpd_order->onOrderChanged();
#endif
    }
  });
}

#if defined(MPRIS_ENABLE)
void MainWindow::setupMpris() {
  mpris = new Mpris(player, global_conf, this);
  connect(mpris, &Mpris::quit, this, &MainWindow::requestQuit);
  connect(mpris, &Mpris::shuffleChanged, this, [=](bool val) {
    ui->orderComboBox->setCurrentIndex(val ? 1 : 0);
    global_conf.savePlaybackOrder(val ? "random" : "sequential");
    global_conf.sync();
  });
}
#endif

#ifdef Q_OS_MACOS
void MainWindow::setupMacMediaControls() {
  mac_media = new MacMediaControls(player, this);
}

void MainWindow::setupMacDockMenu() {
  mac_dock = new MacDockMenu(player, this);
}
#endif

#ifdef SMTC_ENABLE
void MainWindow::setupWindowsMediaControls() {
  win_media = new WindowsMediaControls(player, this, this);
}
#endif

#ifdef Q_OS_WIN
void MainWindow::setupWindowsTaskbar() {
  win_taskbar = new WindowsTaskbar(player, this, this);
  connect(static_cast<MpzApplication *>(qApp), &MpzApplication::paletteChanged, win_taskbar, &WindowsTaskbar::refresh);
}
#endif

void MainWindow::setupFollowCursorCheckbox() {
  ui->followCursorCheckBox->setCheckState(global_conf.playbackFollowCursor() ? Qt::Checked : Qt::Unchecked);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
  connect(ui->followCursorCheckBox, &QCheckBox::checkStateChanged, this, [=](Qt::CheckState state) {
#else
  connect(ui->followCursorCheckBox, &QCheckBox::stateChanged, [=](int state) {
#endif
    global_conf.savePlaybackFollowCursor(state == Qt::Checked);
    global_conf.sync();
  });
}

void MainWindow::setupVolumeControl() {
  volume = new VolumeControl(ui->toolButtonVolume, player->volume(), this);

  connect(volume, &VolumeControl::changed, this, [=](int value) {
    player->setVolume(value);
    volume->setValue(value);
    if (value > 0) {
      local_conf.saveVolume(value);
    }
  });
  connect(player, &Playback::Controller::volumeChanged, volume, &VolumeControl::setValue);
}

void MainWindow::setupDockWidgets() {
  setDockNestingEnabled(true);

  cover_widget = new CoverArt::Widget(this);
  cover_dock = new QDockWidget(tr("Album cover"), this);
  cover_dock->setObjectName("coverArtDock");
  cover_dock->setWidget(cover_widget);
  addDockWidget(Qt::RightDockWidgetArea, cover_dock);

  lyrics_widget = new Lyrics::Widget(this);
  lyrics_dock = new QDockWidget(tr("Lyrics"), this);
  lyrics_dock->setObjectName("lyricsDock");
  lyrics_dock->setWidget(lyrics_widget);
  addDockWidget(Qt::RightDockWidgetArea, lyrics_dock);

  splitDockWidget(cover_dock, lyrics_dock, Qt::Vertical);

  // Hidden by default. restoreState() (in setupUiSettings, called next) reapplies
  // saved visibility/position on later runs; on first run they stay hidden.
  cover_dock->hide();
  lyrics_dock->hide();

  connect(player, &Playback::Controller::started, cover_widget, &CoverArt::Widget::setTrack);
  connect(player, &Playback::Controller::trackChanged, cover_widget, &CoverArt::Widget::setTrack);
  connect(player, &Playback::Controller::stopped, cover_widget, &CoverArt::Widget::clear);

  connect(player, &Playback::Controller::started, lyrics_widget, &Lyrics::Widget::setTrack);
  connect(player, &Playback::Controller::trackChanged, lyrics_widget, &Lyrics::Widget::setTrack);
  connect(player, &Playback::Controller::stopped, lyrics_widget, &Lyrics::Widget::clear);
}

void MainWindow::setupMainMenu() {
  ui->menuButton->setIcon(Icons::get(Icons::Icon::Menu));
  ui->menuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

  main_menu = new MainMenu(ui->menuButton, global_conf, local_conf, modus_operandi);
  main_menu->setViewActions({ cover_dock->toggleViewAction(), lyrics_dock->toggleViewAction() });
  connect(main_menu, &MainMenu::exit, this, &MainWindow::requestQuit);
  connect(main_menu, &MainMenu::toggleTrayIcon, this, &MainWindow::setupTrayIcon);
}

void MainWindow::setupTrayIcon() {
#ifdef Q_OS_MACOS
  // No Qt status item on macOS: Control Center / media keys (MacMediaControls),
  // the Dock menu (MacDockMenu) and the native menu bar already cover it.
  return;
#else
  if (!global_conf.trayIconEnabled()) {
    if (trayicon != nullptr) {
      trayicon->hide();
      trayicon->deleteLater();
    }
    trayicon = nullptr;
    return;
  }
  trayicon = new TrayIcon(this);
  connect(static_cast<MpzApplication *>(qApp), &MpzApplication::paletteChanged, trayicon, &TrayIcon::refreshIcons);
  connect(player, &Playback::Controller::started, trayicon, &TrayIcon::on_playerStarted);
  connect(player, &Playback::Controller::stopped, trayicon, &TrayIcon::on_playerStopped);
  connect(player, &Playback::Controller::paused, trayicon, &TrayIcon::on_playerPaused);
  connect(player, &Playback::Controller::progress, trayicon, &TrayIcon::on_playerProgress);

  connect(trayicon, &TrayIcon::startTriggered, player->controls().play, &QToolButton::click);
  connect(trayicon, &TrayIcon::pauseTriggered, player->controls().pause, &QToolButton::click);
  connect(trayicon, &TrayIcon::stopTriggered, player->controls().stop, &QToolButton::click);
  connect(trayicon, &TrayIcon::nextTriggered, player->controls().next, &QToolButton::click);
  connect(trayicon, &TrayIcon::prevTriggered, player->controls().prev, &QToolButton::click);

  connect(trayicon, &TrayIcon::clicked, this, &MainWindow::toggleHidden);
  connect(trayicon, &TrayIcon::quitTriggered, this, &MainWindow::requestQuit);
#endif
}

void MainWindow::setupPlaybackDispatch() {
  dispatch = new Playback::Dispatch(global_conf, playlists);

  connect(playlist, &PlaylistUi::Controller::selected, this, [=](const Track &track) {
    dispatch->state().setSelected(track.uid());
    dispatch->state().resetFolowedCursor();
#ifdef ENABLE_MPD_SUPPORT
    mpd_order->onTrackSelected(track);
#endif
  });

  connect(player, &Playback::Controller::started, dispatch, &Playback::Dispatch::on_started);
  connect(player, &Playback::Controller::stopped, dispatch, &Playback::Dispatch::on_stopped);
  connect(playlist, &PlaylistUi::Controller::changed, dispatch, &Playback::Dispatch::on_playlistContentChanged);
  connect(playlists, &PlaylistsUi::Controller::removed, dispatch, &Playback::Dispatch::on_playlistContentChanged);

  connect(player, &Playback::Controller::prevRequested, dispatch, &Playback::Dispatch::on_prevRequested);
  connect(player, &Playback::Controller::nextRequested, dispatch, &Playback::Dispatch::on_nextRequested);
  connect(player, &Playback::Controller::startRequested, dispatch, &Playback::Dispatch::on_startRequested);
  connect(player, &Playback::Controller::trackChangedQuery, dispatch, &Playback::Dispatch::on_trackChangedQuery);
  connect(dispatch, &Playback::Dispatch::trackChangedQueryComplete, player, &Playback::Controller::trackChangedQueryComplete);
  connect(dispatch, &Playback::Dispatch::play, player, &Playback::Controller::play);
  connect(dispatch, &Playback::Dispatch::stop, player, &Playback::Controller::stop);
  connect(dispatch, &Playback::Dispatch::unloadPlaylistView, playlist, &PlaylistUi::Controller::on_unload);

  connect(playlists, &PlaylistsUi::Controller::doubleclicked, dispatch, &Playback::Dispatch::on_startFromPlaylistRequested);
}

void MainWindow::setupStatusBar() {
  status_label = new StatusBarLabel(this);
  ui->statusbar->addWidget(status_label);
  connect(player, &Playback::Controller::started, status_label, &StatusBarLabel::on_playerStarted);
  connect(player, &Playback::Controller::stopped, status_label, &StatusBarLabel::on_playerStopped);
  connect(player, &Playback::Controller::paused, status_label, &StatusBarLabel::on_playerPaused);
  connect(player, &Playback::Controller::streamFill, status_label, &StatusBarLabel::on_streamBufferFill);
  connect(player, &Playback::Controller::progress, status_label, &StatusBarLabel::on_progress);

  connect(status_label, &StatusBarLabel::doubleclicked, this, [=]() {
    auto current_track_uid = dispatch->state().playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist != nullptr) {
      playlists->on_jumpTo(current_playlist);
      playlist->on_scrollTo(current_playlist->trackBy(current_track_uid));
    }
  });

#if defined(ENABLE_UPDATE_CHECK)
  ui->statusbar->addWidget(new QWidget(this), 1);
  status_label_update = new QLabel(this);
  status_label_update->setTextFormat(Qt::RichText);
  status_label_update->setTextInteractionFlags(Qt::TextBrowserInteraction);
  status_label_update->setOpenExternalLinks(true);
  status_label_update->hide();
  ui->statusbar->addWidget(status_label_update, 0);
  ui->statusbar->addWidget(new QWidget(this), 1);
#endif

  status_label_right = new QLabel(tr("Nothing selected"), this);
  ui->statusbar->addPermanentWidget(status_label_right);
  connect(playlist, &PlaylistUi::Controller::durationOfSelectedChanged, this, [=](quint32 t) {
    if (t == 0) {
      status_label_right->setText(tr("Nothing selected"));
    } else {
      status_label_right->setText(tr("Selection total duration") + ": " + Track::formattedTime(t));
    }
  });
}

#if defined(ENABLE_UPDATE_CHECK)
void MainWindow::setupUpdateChecker() {
  if (global_conf.disableAutoUpdateCheck()) {
    return;
  }
  update_checker = new UpdateChecker(this);
  connect(update_checker, &UpdateChecker::updateAvailable, this, [this](const QString &version, const QString &url) {
    status_label_update->setText(tr("Update available:") + QString(" <a href=\"%1\">v%2</a>").arg(url, version));
    status_label_update->show();
  });
  QTimer::singleShot(0, this, [this]() { update_checker->check(); });
}
#endif

void MainWindow::setupShortcuts() {
  shortcuts = new Shortcuts(this);

  connect(shortcuts, &Shortcuts::quit, this, &MainWindow::requestQuit);
  connect(shortcuts, &Shortcuts::focusLibrary, this, [=]() {
    ui->treeView->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusPlaylists, this, [=]() {
    ui->listView->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusPlaylist, this, [=]() {
    ui->tableView->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusFilterLibrary, this, [=]() {
    ui->treeViewSearch->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusFilterPlaylists, this, [=]() {
    ui->listViewSearch->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusFilterPlaylist, this, [=]() {
    ui->tableViewSearch->setFocus(Qt::ShortcutFocusReason);
  });

  connect(shortcuts, &Shortcuts::play, this, [&]() {
    if (player->isStopped()) {
      player->controls().play->click();
    }
  });
  connect(shortcuts, &Shortcuts::stop, player->controls().stop, &QToolButton::click);
  connect(shortcuts, &Shortcuts::pause, player->controls().pause, &QToolButton::click);
  connect(shortcuts, &Shortcuts::prev, player->controls().prev, &QToolButton::click);
  connect(shortcuts, &Shortcuts::next, player->controls().next, &QToolButton::click);

  connect(shortcuts, &Shortcuts::openMainMenu, main_menu, &MainMenu::on_open);

  connect(shortcuts, &Shortcuts::openSortMenu, ui->sortButton, &QToolButton::click);

#ifdef ENABLE_DEVICES_MENU
  connect(shortcuts, &Shortcuts::openOutputMenu, ui->toolButtonOutputDevice, &QToolButton::click);
#endif

  auto open_dialog = [=] {
    auto dlg = new ShortcutsDialog(shortcuts, this);
    dlg->setModal(false);
    connect(dlg, &ShortcutsDialog::finished, dlg, &ShortcutsDialog::deleteLater);
    dlg->show();
  };
  connect(main_menu, &MainMenu::openShortcuts, open_dialog);
  connect(shortcuts, &Shortcuts::openShortcutsMenu, open_dialog);
  connect(shortcuts, &Shortcuts::jumpToPLayingTrack, status_label, &StatusBarLabel::doubleclicked);

  connect(shortcuts, &Shortcuts::playPause, this, [=]() {
    if (player->state() == Playback::Controller::Playing) {
      player->controls().pause->click();
    } else {
      player->controls().play->click();
    }
  });
  connect(shortcuts, &Shortcuts::volumeUp, this, [=]() {
    player->setVolume(qMin(100, player->volume() + 5));
  });
  connect(shortcuts, &Shortcuts::volumeDown, this, [=]() {
    player->setVolume(qMax(0, player->volume() - 5));
  });
  connect(shortcuts, &Shortcuts::openSettings, this, [this]() {
    SettingsDialog dlg(global_conf, local_conf, this);
    connect(&dlg, &SettingsDialog::trayIconToggled, this, &MainWindow::setupTrayIcon);
    dlg.exec();
  });
}

void MainWindow::setupWindowTitle() {
  setWindowTitle(qApp->applicationDisplayName());
  connect(player, &Playback::Controller::started, this, [=](const Track &track) {
    setWindowTitle("[" + track.shortText() + "] " + qApp->applicationDisplayName());
  });
  connect(player, &Playback::Controller::stopped, this, [=]() {
    setWindowTitle(qApp->applicationDisplayName());
  });
}

void MainWindow::setupPlaybackLog() {
  playback_log = new PlaybackLogUi::Controller(local_conf, global_conf, this);
  connect(main_menu, &MainMenu::openPlaybackLog, playback_log, &PlaybackLogUi::Controller::showWindow);
  connect(player, &Playback::Controller::started, playback_log, &PlaybackLogUi::Controller::append);
  connect(player, &Playback::Controller::trackChanged, playback_log, &PlaybackLogUi::Controller::append);

  connect(playback_log, &PlaybackLogUi::Controller::jumpToTrack, this, [=](quint64 track_uid) {
    auto plst = playlists->playlistByTrackUid(track_uid);
    if (plst != nullptr) {
      playlists->on_jumpTo(plst);
      playlist->on_scrollTo(plst->trackBy(track_uid));
    }
  });

  connect(shortcuts, &Shortcuts::openPlabackLog, playback_log, &PlaybackLogUi::Controller::showWindow);
  connect(status_label, &StatusBarLabel::showPlaybackLog, playback_log, &PlaybackLogUi::Controller::showWindow);
  connect(player, &Playback::Controller::monotonicPlaybackTimerIncrement, playback_log, &PlaybackLogUi::Controller::on_monotonicPlaybackTimeIncrement);
}

void MainWindow::setupSortMenu() {
  sort_menu = new SortUi::SortMenu(ui->sortButton, global_conf);

  ui->sortButton->setIcon(Icons::get(Icons::Icon::Sort));

  connect(sort_menu, &SortUi::SortMenu::triggered, playlist, &PlaylistUi::Controller::sortBy);
}

void MainWindow::setupSleepLock() {
  if (!global_conf.inhibitSleepWhilePlaying()) {
    sleep_lock = nullptr;
    return;
  }

  sleep_lock = new SleepLock(this);
  connect(player, &Playback::Controller::started, this, [=](Track _track) {
    Q_UNUSED(_track);
    sleep_lock->activate(true);
  });
  connect(player, &Playback::Controller::paused, this, [=](Track _track) {
    Q_UNUSED(_track);
    sleep_lock->activate(false);
  });
  connect(player, &Playback::Controller::stopped, this, [=]() {
    sleep_lock->activate(false);
  });
}

void MainWindow::setupOutputDevice() {
#ifdef ENABLE_DEVICES_MENU
  connect(ui->toolButtonOutputDevice, &QToolButton::clicked, this, [=]() {
    AudioDeviceUi::DevicesMenu device_menu(this, local_conf);
    connect(&device_menu, &AudioDeviceUi::DevicesMenu::outputDeviceChanged, player, &Playback::Controller::setOutputDevice);
    int menu_width = device_menu.sizeHint().width();
    int x = ui->toolButtonOutputDevice->width() - menu_width;
    int y = ui->toolButtonOutputDevice->height();
    QPoint pos(ui->toolButtonOutputDevice->mapToGlobal(QPoint(x, y)));
    device_menu.exec(pos);
  });
  connect(&modus_operandi, &ModusOperandi::changed, this, [=](auto mode) {
    ui->toolButtonOutputDevice->setEnabled(mode == ModusOperandi::MODUS_LOCALFS);
  });
  ui->toolButtonOutputDevice->setEnabled(modus_operandi.get() == ModusOperandi::MODUS_LOCALFS);
  ui->toolButtonOutputDevice->setIcon(Icons::get(Icons::Icon::Headphones));
  ui->toolButtonOutputDevice->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#else
  ui->toolButtonOutputDevice->setVisible(false);
#endif
}

#ifdef ENABLE_MPD_SUPPORT
void MainWindow::setupMpdOrder() {
  mpd_order = new Playback::Mpd::PlaybackOrder(global_conf, modus_operandi.mpd_client, playlists, dispatch, this);
  connect(player, &Playback::Controller::started, mpd_order, &Playback::Mpd::PlaybackOrder::updateByTrack);
}
#endif

void MainWindow::preloadPlaylist(const QStringList &args) {
  if (modus_operandi.get() != ModusOperandi::MODUS_LOCALFS) {
    return;
  }

  QList<QDir> preload_files;
  for (const auto &i : std::as_const(args)) {
    preload_files << QDir(i);
  }
  if (preload_files.isEmpty()) {
    return;
  }

  QEventLoop loop;
  std::shared_ptr<Playlist::Playlist> pl;
  auto conn = connect(playlists, &PlaylistsUi::Controller::loaded, this, [&](const std::shared_ptr<Playlist::Playlist> item) {
    pl = item;
    loop.quit();
  });

  QTimer::singleShot(60000, &loop, &QEventLoop::quit); // deadline: don't hang startup if loaded never fires
  emit library->createNewPlaylist(preload_files, "");
  loop.exec();
  if (pl != nullptr && pl->tracks().size() > 0) {
    emit playlist->activated(pl->tracks().first());
  }
  disconnect(conn);
}

#ifdef Q_OS_MACOS
void MainWindow::setupMacMenuBar() {
  mac_menubar = new MacMenuBar(this, global_conf, local_conf, shortcuts, player, modus_operandi, sort_menu,
                               cover_dock->toggleViewAction(), lyrics_dock->toggleViewAction());
}
#endif
