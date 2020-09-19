﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "config/storage.h"
#include "waitingspinnerwidget.h"

#include <QDebug>
#include <QApplication>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), trayicon(nullptr) {
#if defined(MPRIS_ENABLE)
  mpris = nullptr;
#endif
  ui->setupUi(this);
  setWindowIcon(QIcon(":/icons/icons/mpz.png"));

  spinner = new BusySpinner(ui->widgetSpinner, this);

  library = new DirectoryUi::Controller(ui->treeView, ui->treeViewSearch, ui->comboBoxLibraries, ui->toolButtonLibraries, local_conf, this);
  playlists = new PlaylistsUi::Controller(ui->listView, ui->listViewSearch, local_conf, spinner, this);
  playlist = new PlaylistUi::Controller(ui->tableView, ui->tableViewSearch, local_conf, this);

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
  player = new Playback::Controller(pc, this);
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

  loadUiSettings();

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
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::loadUiSettings() {
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
}

void MainWindow::setupOrderCombobox() {
  ui->orderComboBox->addItem("sequential");
  ui->orderComboBox->addItem("random");
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
  ui->perPlaylistOrdercomboBox->addItem("(use global)");
  ui->perPlaylistOrdercomboBox->addItem("random");
  ui->perPlaylistOrdercomboBox->addItem("sequential");
  connect(playlists, &PlaylistsUi::Controller::selected, [=](const std::shared_ptr<Playlist::Playlist> playlist) {
    if (playlist->random() == Playlist::Playlist::Random) {
      ui->perPlaylistOrdercomboBox->setCurrentIndex(1);
    } else if (playlist->random() == Playlist::Playlist::Sequential) {
      ui->perPlaylistOrdercomboBox->setCurrentIndex(2);
    } else if (playlist->random() == Playlist::Playlist::None) {
      ui->perPlaylistOrdercomboBox->setCurrentIndex(0);
    }
  });
  connect(ui->perPlaylistOrdercomboBox, QOverload<int>::of(&QComboBox::activated), [=](int idx) {
    auto current_playlist = playlists->playlistByTrackUid(dispatch->state().playingTrack());
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
  main_menu = new MainMenu(ui->menuButton, global_conf);
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
  trayicon = new TrayIcon(this);
  connect(player, &Playback::Controller::started, trayicon, &TrayIcon::on_playerStarted);
  connect(player, &Playback::Controller::stopped, trayicon, &TrayIcon::on_playerStopped);
  connect(player, &Playback::Controller::paused, trayicon, &TrayIcon::on_playerPaused);
  connect(player, &Playback::Controller::progress, trayicon, &TrayIcon::on_playerProgress);

  connect(trayicon, &TrayIcon::startTriggered, player->controls().play, &QToolButton::click);
  connect(trayicon, &TrayIcon::pauseTriggered, player->controls().pause, &QToolButton::click);
  connect(trayicon, &TrayIcon::stopTriggered, player->controls().stop, &QToolButton::click);
  connect(trayicon, &TrayIcon::nextTriggered, player->controls().next, &QToolButton::click);
  connect(trayicon, &TrayIcon::prevTriggered, player->controls().prev, &QToolButton::click);
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
}

void MainWindow::setupStatusBar() {
  status_label = new StatusBarLabel(ui->statusbar, this);
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
}

void MainWindow::setupShortcuts() {
  shortcuts = new Shortcuts(this, global_conf);

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

