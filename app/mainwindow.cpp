﻿#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "config/storage.h"
#include "waitingspinnerwidget.h"
#include "QHotkey/qhotkey.h"

#include <QDebug>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), trayicon(nullptr) {
  while (true) {
    pla.play();
  }

  #if defined(MPRIS_ENABLE)
    mpris = nullptr;
  #endif
  ui->setupUi(this);
  setWindowTitle(qApp->applicationName());
  setWindowIcon(QIcon(":/icons/icons/appicon.png"));

  spinner = new BusySpinner(ui->widgetSpinner, this);

  library = new DirectoryUi::Controller(ui->treeView, ui->treeViewSearch, ui->comboBoxLibraries, ui->toolButtonLibraries, local_conf, this);
  playlists = new PlaylistsUi::Controller(ui->listView, ui->listViewSearch, local_conf, spinner, this);
  playlist = new PlaylistUi::Controller(ui->tableView, ui->tableViewSearch, local_conf, this);

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

  loadUiSettings();

  setupPlaybackDispatch();
  setupStatusBar();
  setupTrayIcon();
  setupOrderCombobox();
  setupFollowCursorCheckbox();
  setupVolumeControl();
  setupMainMenu();
  setupMediaKeys();
#if defined(MPRIS_ENABLE)
  setupMpris();
#endif

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
  ui->orderComboBox->addItem("Sequential");
  ui->orderComboBox->addItem("Random");
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
  main_menu = new MainMenu(ui->menuButton, global_conf, this);
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

  connect(status_label, &StatusBarLabel::doubleclicked, [=]() {
    auto current_track_uid = dispatch->state().playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist != nullptr) {
      playlists->on_jumpTo(current_playlist);
      playlist->on_scrollTo(current_playlist->trackBy(current_track_uid));
    }
  });
}

void MainWindow::setupMediaKeys() {
  auto play = new QHotkey(Qt::Key_MediaPlay, Qt::NoModifier, true, this);
  connect(play, &QHotkey::activated, [&]() {
    if (player->isStopped()) {
      player->controls().play->click();
    }
  });

  auto stop = new QHotkey(Qt::Key_MediaStop, Qt::NoModifier, true, this);
  connect(stop, &QHotkey::activated, [&]() {
    player->controls().stop->click();
  });

  auto pause = new QHotkey(Qt::Key_MediaPause, Qt::NoModifier, true, this);
  connect(pause, &QHotkey::activated, [&]() {
    player->controls().pause->click();
  });

  auto prev = new QHotkey(Qt::Key_MediaPrevious, Qt::NoModifier, true, this);
  connect(prev, &QHotkey::activated, [&]() {
    player->controls().prev->click();
  });

  auto next = new QHotkey(Qt::Key_MediaNext, Qt::NoModifier, true, this);
  connect(next, &QHotkey::activated, [&]() {
    player->controls().next->click();
  });
}

