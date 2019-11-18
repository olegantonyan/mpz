#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "config/storage.h"

#include <QDebug>
#include <QApplication>
#include <memory>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <statusbarlabel.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowTitle("mpz");

  library = new DirectoryUi::View(ui->treeView, "/mnt/storage/music", this);
  playlists = new PlaylistsUi::View(ui->listView, local_conf, this);
  playlist = new PlaylistUi::View(ui->tableView, this);

  auto pc = Playback::Controls();
  pc.next = ui->nextButton;
  pc.prev = ui->prevButton;
  pc.stop = ui->stopButton;
  pc.play = ui->playButton;
  pc.pause = ui->pauseButton;
  pc.seekbar = ui->progressBar;
  pc.time = ui->timeLabel;
  player = new Playback::View(pc, this);

  setupControllerLogic();

  loadUiSettings();

  playlists->load();

  auto status_label = new StatusBarLabel(this);
  ui->statusbar->addWidget(status_label);
  status_label->setText("Stopped");
  connect(status_label, &StatusBarLabel::doubleclicked, [=]() {
    auto current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    playlists->on_jumpTo(current_playlist);
    playlist->on_scrollTo(current_playlist->trackBy(current_track_uid));
  });

  connect(player, &Playback::View::started, [=](const Track &track) {
    status_label->setText(QString("Playing ") + track.filename() + " | " + track.formattedAudioInfo());
  });
  connect(player, &Playback::View::stopped, [=]() {
    status_label->setText("Stopped");
  });
  connect(player, &Playback::View::paused, [=](const Track &track) {
    status_label->setText(QString("Paused ") + track.filename() + " | " + track.formattedAudioInfo());
  });



  ui->tableView->setFocus();
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::loadUiSettings() {
  restoreGeometry(local_conf.windowGeomentry());
  restoreState(local_conf.windowState());

  connect(ui->splitter, &QSplitter::splitterMoved, [=](int pos, int index) {
    (void)pos;
    (void)index;
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

void MainWindow::setupControllerLogic() {
  connect(library, &DirectoryUi::View::createNewPlaylist, playlists, &PlaylistsUi::View::on_createPlaylist);
  connect(library, &DirectoryUi::View::appendToCurrentPlaylist, playlists, &PlaylistsUi::View::on_appendToCurrentPlaylist);
  connect(playlists, &PlaylistsUi::View::selected, playlist, &PlaylistUi::View::on_load);
  connect(playlists, &PlaylistsUi::View::emptied, playlist, &PlaylistUi::View::on_unload);
  connect(playlist, &PlaylistUi::View::activated, player, &Playback::View::play);
  connect(player, &Playback::View::started, playlist, &PlaylistUi::View::on_start);
  connect(player, &Playback::View::stopped, playlist, &PlaylistUi::View::on_stop);

  connect(playlist, &PlaylistUi::View::selected, [=](const Track &track) {
    player_state.setSelected(track.uid());
  });

  connect(player, &Playback::View::started, [=](const Track &track) {
    player_state.setPlaying(track.uid());
  });

  connect(player, &Playback::View::stopped, [=]() {
    player_state.resetPlaying();
  });

  connect(player, &Playback::View::prev_requested, [=]() {
    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto prev = current - 1;
    if (prev < 0) {
      auto max = current_playlist->tracks().size() - 1;
      Track t = current_playlist->tracks().at(max);
      player->play(t);
    } else {
      Track t = current_playlist->tracks().at(prev);
      player->play(t);
    }
  });

  connect(player, &Playback::View::next_requested, [=]() {
    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto next = current + 1;
    if (next > current_playlist->tracks().size() - 1) {
      Track t = current_playlist->tracks().at(0);
      player->play(t);
    } else {
      Track t = current_playlist->tracks().at(next);
      player->play(t);
    }
  });

  connect(player, &Playback::View::start_requested, [=]() {
    quint64 selected_track_uid = player_state.selectedTrack();
    auto selected_playlist = playlists->playlistByTrackUid(selected_track_uid);
    if (selected_playlist != nullptr) {
      Track t = selected_playlist->trackBy(selected_track_uid);
      player->play(t);
    }
  });
}

void MainWindow::closeEvent(QCloseEvent *event) {
  local_conf.saveWindowGeometry(saveGeometry());
  local_conf.saveWindowState(saveState());
  local_conf.sync();
  //global_conf.sync();
  QMainWindow::closeEvent(event);
}

void MainWindow::on_menuButton_clicked() {
  QMenu menu;
  QAction github("Github");
  connect(&github, &QAction::triggered, [=]() {
    QDesktopServices::openUrl(QUrl("https://github.com/olegantonyan/mpz"));
  });

  menu.addAction(&github);

  int menu_width = menu.sizeHint().width();
  int x = ui->menuButton->width() - menu_width;
  int y = ui->menuButton->height();
  QPoint pos(ui->menuButton->mapToGlobal(QPoint(x, y)));

  menu.exec(pos);
}
