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

  connect(library, &DirectoryUi::View::createNewPlaylist, playlists, &PlaylistsUi::View::on_createPlaylist);
  connect(playlists, &PlaylistsUi::View::selected, playlist, &PlaylistUi::View::on_load);
  connect(playlists, &PlaylistsUi::View::emptied, playlist, &PlaylistUi::View::on_unload);
  connect(playlists, &PlaylistsUi::View::activated, player, &Playback::View::play);
  connect(playlist, &PlaylistUi::View::activated, playlists, &PlaylistsUi::View::on_trackActivated);
  connect(playlist, &PlaylistUi::View::selected, playlists, &PlaylistsUi::View::on_trackSelected);
  connect(player, &Playback::View::prev_requested, playlists, &PlaylistsUi::View::on_prevRequested);
  connect(player, &Playback::View::next_requested, playlists, &PlaylistsUi::View::on_nextRequested);
  connect(player, &Playback::View::start_requested, playlists, &PlaylistsUi::View::on_startRequested);
  connect(player, &Playback::View::started, playlists, &PlaylistsUi::View::on_started);
  connect(player, &Playback::View::stopped, playlists, &PlaylistsUi::View::on_stopped);
  connect(playlists, &PlaylistsUi::View::highlighted, playlist, &PlaylistUi::View::highlight);

  loadUiSettings();

  playlists->load();

  auto status_label = new StatusBarLabel(this);
  ui->statusbar->addWidget(status_label);
  status_label->setText("Stopped");
  connect(status_label, &StatusBarLabel::doubleclicked, [=]() {
    //auto t = player->current_track().track;
    //if (t.isValid()) {
      qDebug() << "TODO: jump to playlist";
    //}
  });

  connect(player, &Playback::View::started, [=](const TrackWrapper &track) {
    status_label->setText(QString("Playing ") + track.track.filename() + " | " + track.track.formattedAudioInfo());
  });
  connect(player, &Playback::View::stopped, [=]() {
    status_label->setText("Stopped");
  });
  connect(player, &Playback::View::paused, [=](const TrackWrapper &track) {
    status_label->setText(QString("Paused ") + track.track.filename() + " | " + track.track.formattedAudioInfo());
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
