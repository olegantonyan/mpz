#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "shortcuts_ui/shortcutsdialog.h"

#include <QDebug>
#include <QApplication>
#include <QStyle>
#include <QEvent>
#include <QTableWidget>
#include <QTableWidgetItem>

MainWindow::MainWindow(const QStringList &args, IPC::Instance *instance, Config::Local &local_c, Config::Global &global_c, QWidget *parent) :
  QMainWindow(parent), ui(new Ui::MainWindow), local_conf(local_c), global_conf(global_c) {
  trayicon = nullptr;
#if defined(MPRIS_ENABLE)
  mpris = nullptr;
#endif
  ui->setupUi(this);
  setWindowIcon(QIcon(":/app/resources/icons/64x64/mpz.png"));

  spinner = new BusySpinner(ui->widgetSpinner, this);

  library = new DirectoryUi::Controller(ui->treeView, ui->treeViewSearch, ui->comboBoxLibraries, ui->toolButtonLibraries, ui->toolButtonLibrarySort, local_conf, this);
  playlists = new PlaylistsUi::Controller(ui->listView, ui->listViewSearch, local_conf, spinner, this);
  playlist = new PlaylistUi::Controller(ui->tableView, ui->tableViewSearch, spinner, local_conf, global_conf, this);

  ui->toolButtonLibrarySort->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));

  ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
  ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  ui->pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
  ui->nextButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
  ui->prevButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
  Playback::Controls pc;
  pc.next = ui->nextButton;
  pc.prev = ui->prevButton;
  pc.stop = ui->stopButton;
  pc.play = ui->playButton;
  pc.pause = ui->pauseButton;
  pc.seekbar = ui->progressBar;
  pc.time = ui->timeLabel;
  player = new Playback::Controller(pc, streamBuffer(), this);
  if (local_conf.volume() > 0) {
    player->setVolume(local_conf.volume());
  }

  connect(library, &DirectoryUi::Controller::createNewPlaylist, playlists, &PlaylistsUi::Controller::on_createPlaylist);
  connect(library, &DirectoryUi::Controller::appendToCurrentPlaylist, playlist, &PlaylistUi::Controller::on_appendToPlaylist);
  connect(playlists, &PlaylistsUi::Controller::selected, playlist, &PlaylistUi::Controller::on_load);
  connect(playlists, &PlaylistsUi::Controller::emptied, playlist, &PlaylistUi::Controller::on_unload);
  connect(playlist, &PlaylistUi::Controller::activated, player, &Playback::Controller::play);
  connect(player, &Playback::Controller::started, playlist, &PlaylistUi::Controller::on_start);
  connect(player, &Playback::Controller::paused, playlist, &PlaylistUi::Controller::on_pause);
  connect(player, &Playback::Controller::stopped, playlist, &PlaylistUi::Controller::on_stop);
  connect(playlist, &PlaylistUi::Controller::changed, playlists, &PlaylistsUi::Controller::on_playlistChanged);
  connect(player, &Playback::Controller::started, playlists, &PlaylistsUi::Controller::on_start);
  connect(player, &Playback::Controller::stopped, playlists, &PlaylistsUi::Controller::on_stop);

  setupUiSettings();

  setupPlaybackDispatch();
  setupStatusBar();
  setupTrayIcon();
  setupOrderCombobox();
  setupPerPlaylistOrderCombobox();
  setupFollowCursorCheckbox();
  setupVolumeControl();
  setupMainMenu();
#if defined(MPRIS_ENABLE)
  setupMpris();
#endif
  setupShortcuts();
  setupWindowTitle();
  setupPlaybackLog();
  setupSortMenu();
  setupSleepLock();

  preloadPlaylist(args);

  connect(instance, &IPC::Instance::load_files_received, [=](const QStringList &lst) {
    preloadPlaylist(lst);
    show();
    raise();
    setFocus();
  });
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::toggleHidden() {
  if (isHidden()) {
    show();
    raise();
    setFocus();
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

  connect(ui->splitter, &QSplitter::splitterMoved, [=](int pos, int index) {
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
  player->stop();
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

void MainWindow::changeEvent(QEvent *event) {
  if(global_conf.minimizeToTray() && event->type() == QEvent::WindowStateChange && isMinimized()) {
    hide();
  }
  QMainWindow::changeEvent(event);
}

void MainWindow::setupOrderCombobox() {
  ui->orderComboBox->addItem(tr("sequential"));
  ui->orderComboBox->addItem(tr("random"));
  ui->orderComboBox->setCurrentIndex(global_conf.playbackOrder() == "random" ? 1 : 0);
  connect(ui->orderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) {
    global_conf.savePlaybackOrder(idx == 1 ? "random" : "sequential");
    global_conf.sync();
#if defined(MPRIS_ENABLE)
    if (mpris) {
      mpris->on_shuffleChanged(idx == 1);
    }
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
  connect(playlists, &PlaylistsUi::Controller::selected, [=](const std::shared_ptr<Playlist::Playlist> playlist) {
    if (playlist != nullptr) {
      if (playlist->random() == Playlist::Playlist::Random) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(1);
      } else if (playlist->random() == Playlist::Playlist::Sequential) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(2);
      } else if (playlist->random() == Playlist::Playlist::None) {
        ui->perPlaylistOrdercomboBox->setCurrentIndex(0);
      }
    }
  });
  connect(ui->perPlaylistOrdercomboBox, QOverload<int>::of(&QComboBox::activated), [=](int idx) {
    auto current_playlist = playlists->playlistByTrackUid(dispatch->state().selectedTrack());
    if (current_playlist != nullptr) {
      current_playlist->setRandom(static_cast<Playlist::Playlist::PlaylistRandom>(idx));
      playlists->on_playlistChanged(current_playlist);
    }
  });
}

#if defined(MPRIS_ENABLE)
void MainWindow::setupMpris() {
  mpris = new Mpris(player, this);
  connect(mpris, &Mpris::quit, this, &QMainWindow::close);
  connect(mpris, &Mpris::shuffleChanged, [=](bool val) {
    ui->orderComboBox->setCurrentIndex(val ? 1 : 0);
    global_conf.savePlaybackOrder(val ? "random" : "sequential");
    global_conf.sync();
  });
}
#endif

void MainWindow::setupFollowCursorCheckbox() {
  ui->followCursorCheckBox->setCheckState(global_conf.playbackFollowCursor() ? Qt::Checked : Qt::Unchecked);
  connect(ui->followCursorCheckBox, &QCheckBox::stateChanged, [=](int state) {
    global_conf.savePlaybackFollowCursor(state == Qt::Checked);
    global_conf.sync();
  });
}

void MainWindow::setupVolumeControl() {
  volume = new VolumeControl(ui->toolButtonVolume, player->volume(), this);

  connect(volume, &VolumeControl::changed, [=](int value) {
    player->setVolume(value);
    volume->setValue(value);
    if (value > 0) {
      local_conf.saveVolume(value);
    }
  });
  connect(player, &Playback::Controller::volumeChanged, volume, &VolumeControl::setValue);
}

void MainWindow::setupMainMenu() {
  ui->menuButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
  main_menu = new MainMenu(ui->menuButton, global_conf, local_conf);
  connect(main_menu, &MainMenu::exit, this, &MainWindow::close);
  connect(main_menu, &MainMenu::toggleTrayIcon, this, &MainWindow::setupTrayIcon);
}

void MainWindow::setupTrayIcon() {
  if (!global_conf.trayIconEnabled()) {
    if (trayicon != nullptr) {
      trayicon->hide();
      trayicon->deleteLater();
    }
    trayicon = nullptr;
    return;
  }
  trayicon = new TrayIcon(this, global_conf);
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
}

void MainWindow::setupPlaybackDispatch() {
  dispatch = new Playback::Dispatch(global_conf, playlists);

  connect(playlist, &PlaylistUi::Controller::selected, [=](const Track &track) {
    dispatch->state().setSelected(track.uid());
    dispatch->state().resetFolowedCursor();
  });

  connect(player, &Playback::Controller::started, [=](const Track &track) {
    dispatch->state().setPlaying(track.uid());
    dispatch->state().setFollowedCursor();
  });

  connect(player, &Playback::Controller::stopped, [=]() {
    dispatch->state().resetPlaying();
  });

  connect(player, &Playback::Controller::prevRequested, dispatch, &Playback::Dispatch::on_prevRequested);
  connect(player, &Playback::Controller::nextRequested, dispatch, &Playback::Dispatch::on_nextRequested);
  connect(player, &Playback::Controller::startRequested, dispatch, &Playback::Dispatch::on_startRequested);
  connect(dispatch, &Playback::Dispatch::play, player, &Playback::Controller::play);

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

  connect(status_label, &StatusBarLabel::doubleclicked, [=]() {
    auto current_track_uid = dispatch->state().playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist != nullptr) {
      playlists->on_jumpTo(current_playlist);
      playlist->on_scrollTo(current_playlist->trackBy(current_track_uid));
    }
  });

  status_label_right = new QLabel(tr("Nothing selected"), this);
  ui->statusbar->addPermanentWidget(status_label_right);
  connect(playlist, &PlaylistUi::Controller::durationOfSelectedChanged, [=](quint32 t) {
    if (t == 0) {
      status_label_right->setText(tr("Nothing selected"));
    } else {
      status_label_right->setText(tr("Selection total duration") + ": " + Track::formattedTime(t));
    }
  });
}

void MainWindow::setupShortcuts() {
  shortcuts = new Shortcuts(this);

  connect(shortcuts, &Shortcuts::quit, this, &QMainWindow::close);
  connect(shortcuts, &Shortcuts::focusLibrary, [=]() {
    ui->treeView->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusPlaylists, [=]() {
    ui->listView->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusPlaylist, [=]() {
    ui->tableView->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusFilterLibrary, [=]() {
    ui->treeViewSearch->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusFilterPlaylists, [=]() {
    ui->listViewSearch->setFocus(Qt::ShortcutFocusReason);
  });
  connect(shortcuts, &Shortcuts::focusFilterPlaylist, [=]() {
    ui->tableViewSearch->setFocus(Qt::ShortcutFocusReason);
  });

  connect(shortcuts, &Shortcuts::play, [&]() {
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

  auto open_dialog = [=] {
    auto dlg = new ShortcutsDialog(shortcuts, this);
    dlg->setModal(false);
    connect(dlg, &ShortcutsDialog::finished, dlg, &ShortcutsDialog::deleteLater);
    dlg->show();
  };
  connect(main_menu, &MainMenu::openShortcuts, open_dialog);
  connect(shortcuts, &Shortcuts::openShortcutsMenu, open_dialog);
  connect(shortcuts, &Shortcuts::jumpToPLayingTrack, status_label, &StatusBarLabel::doubleclicked);
}

void MainWindow::setupWindowTitle() {
  setWindowTitle(qApp->applicationDisplayName());
  connect(player, &Playback::Controller::started, [=](const Track &track) {
    setWindowTitle("[" + track.shortText() + "] " + qApp->applicationDisplayName());
  });
  connect(player, &Playback::Controller::stopped, [=]() {
    setWindowTitle(qApp->applicationDisplayName());
  });
}

void MainWindow::setupPlaybackLog() {
  playback_log = new PlaybackLogUi::Controller(local_conf, global_conf, this);
  connect(main_menu, &MainMenu::openPlaybackLog, playback_log, &PlaybackLogUi::Controller::showWindow);
  connect(player, &Playback::Controller::started, playback_log, &PlaybackLogUi::Controller::append);
  connect(player, &Playback::Controller::trackChanged, playback_log, &PlaybackLogUi::Controller::append);

  connect(playback_log, &PlaybackLogUi::Controller::jumpToTrack, [=](quint64 track_uid) {
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

  ui->sortButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));

  connect(sort_menu, &SortUi::SortMenu::triggered, playlist, &PlaylistUi::Controller::sortBy);
}

void MainWindow::setupSleepLock() {
  if (!global_conf.inhibitSleepWhilePlaying()) {
    sleep_lock = nullptr;
    return;
  }

  sleep_lock = new SleepLock(this);
  connect(player, &Playback::Controller::started, [=](Track _track) {
    Q_UNUSED(_track);
    sleep_lock->activate(true);
  });
  connect(player, &Playback::Controller::paused, [=](Track _track) {
    Q_UNUSED(_track);
    sleep_lock->activate(false);
  });
  connect(player, &Playback::Controller::stopped, [=]() {
    sleep_lock->activate(false);
  });
}

void MainWindow::preloadPlaylist(const QStringList &args) {
  QList<QDir> preload_files;
  for (auto i : args) {
    preload_files << QDir(i);
  }
  if (preload_files.isEmpty()) {
    return;
  }

  QEventLoop loop;
  std::shared_ptr<Playlist::Playlist> pl;
  auto conn = connect(playlists, &PlaylistsUi::Controller::loaded, [&](const std::shared_ptr<Playlist::Playlist> item) {
    pl = item;
    loop.quit();
  });

  emit library->createNewPlaylist(preload_files);
  loop.exec();
  if (pl != nullptr && pl->tracks().size() > 0) {
    emit playlist->activated(pl->tracks().first());
  }
  disconnect(conn);
}
