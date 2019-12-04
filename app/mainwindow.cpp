#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "config/storage.h"
#include "waitingspinnerwidget.h"

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

  spinner = new BusySpinner(ui->widgetSpinner, this);

  library = new DirectoryUi::View(ui->treeView, local_conf, this);
  playlists = new PlaylistsUi::View(ui->listView, local_conf, spinner, this);
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

  dispatch = new Playback::Dispatch(global_conf, playlists);

  connect(library, &DirectoryUi::View::createNewPlaylist, playlists, &PlaylistsUi::View::on_createPlaylist);
  connect(library, &DirectoryUi::View::appendToCurrentPlaylist, playlist, &PlaylistUi::View::on_appendToPlaylist);
  connect(playlists, &PlaylistsUi::View::selected, playlist, &PlaylistUi::View::on_load);
  connect(playlists, &PlaylistsUi::View::emptied, playlist, &PlaylistUi::View::on_unload);
  connect(playlist, &PlaylistUi::View::activated, player, &Playback::View::play);
  connect(player, &Playback::View::started, playlist, &PlaylistUi::View::on_start);
  connect(player, &Playback::View::stopped, playlist, &PlaylistUi::View::on_stop);
  connect(playlist, &PlaylistUi::View::changed, playlists, &PlaylistsUi::View::on_playlistChanged);

  connect(playlist, &PlaylistUi::View::selected, [=](const Track &track) {
    dispatch->state().setSelected(track.uid());
    dispatch->state().resetFolowedCursor();
  });

  connect(player, &Playback::View::started, [=](const Track &track) {
    dispatch->state().setPlaying(track.uid());
    dispatch->state().setFollowedCursor();
  });

  connect(player, &Playback::View::stopped, [=]() {
    dispatch->state().resetPlaying();
  });

  connect(player, &Playback::View::prevRequested, dispatch, &Playback::Dispatch::on_prevRequested);
  connect(player, &Playback::View::nextRequested, dispatch, &Playback::Dispatch::on_nextRequested);
  connect(player, &Playback::View::startRequested, dispatch, &Playback::Dispatch::on_startRequested);
  connect(dispatch, &Playback::Dispatch::play, player, &Playback::View::play);

  loadUiSettings();

  playlists->load();

  auto status_label = new StatusBarLabel(this);
  ui->statusbar->addWidget(status_label);
  status_label->setText("Stopped");
  connect(status_label, &StatusBarLabel::doubleclicked, [=]() {
    auto current_track_uid = dispatch->state().playingTrack();
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



  auto appicon = QIcon(":/icons/icons/appicon.png");
  setWindowIcon(appicon);
  trayicon = new TrayIcon(player, appicon, this);
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
  global_conf.sync();
  trayicon->hide();
  QMainWindow::closeEvent(event);
}

void MainWindow::on_menuButton_clicked() {
  QMenu menu;
  QAction github("Github");
  QAction quit("Quit");
  connect(&github, &QAction::triggered, [=]() {
    QDesktopServices::openUrl(QUrl("https://github.com/olegantonyan/mpz"));
  });
  connect(&quit, &QAction::triggered, this, &QMainWindow::close);

  menu.addAction(&github);
  menu.addSeparator();
  menu.addAction(&quit);

  int menu_width = menu.sizeHint().width();
  int x = ui->menuButton->width() - menu_width;
  int y = ui->menuButton->height();
  QPoint pos(ui->menuButton->mapToGlobal(QPoint(x, y)));

  menu.exec(pos);
}

