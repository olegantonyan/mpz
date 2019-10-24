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
  connect(playlist, &PlaylistUi::View::activated, player, &Playback::View::play);
  connect(player, &Playback::View::prev_requested, playlist, &PlaylistUi::View::on_prev_requested);
  connect(player, &Playback::View::next_requested, playlist, &PlaylistUi::View::on_next_requested);
  connect(player, &Playback::View::start_requested, playlist, &PlaylistUi::View::on_start_requested);
  connect(player, &Playback::View::started, playlist, &PlaylistUi::View::on_started);
  connect(player, &Playback::View::stopped, playlist, &PlaylistUi::View::on_stopped);


  loadUiSettings();

  playlists->load();

  auto status_label = new StatusBarLabel(this);
  ui->statusbar->addWidget(status_label);
  status_label->setText("Stopped");
  connect(status_label, &StatusBarLabel::doubleclicked, [=]() {
    auto t = player->current_track();
    if (t.track.isValid()) {
      qDebug() << "TODO: jump to playlist" << t.plalist_index << "track" << t.track_index;
    }
  });

  connect(player, &Playback::View::started, [=](const TrackWrapper &track) {
    status_label->setText(QString("Playing: ") + track.track.filename() + " | " + track.track.formattedAudioInfo());
    //ui->statusbar->showMessage(QString("Playing ") + track.track.filename() + " | " + track.track.formattedAudioInfo());
  });
  connect(player, &Playback::View::stopped, [=]() {
    status_label->setText("Stopped");
    //ui->statusbar->showMessage("Stopped");
  });
  connect(player, &Playback::View::paused, [=](const TrackWrapper &track) {
    status_label->setText(QString("Paused: ") + track.track.filename() + " | " + track.track.formattedAudioInfo());
    //ui->statusbar->showMessage(QString("Paused ") + track.track.filename() + " | " + track.track.formattedAudioInfo());
  });



  ui->tableView->setFocus();

/*
  QMap<QString, Config::Value> m;
  m.insert("hello", Config::Value(123));
  m.insert("fuch", Config::Value(10));
  local_conf.storage.set("sdafas", Config::Value(m));

  QList<Config::Value> l;
  l.append(Config::Value(m));
  l.append(Config::Value(m));
  auto i = Config::Value(l);
  //i.setListType(Config::Value::Type::Map);
local_conf.storage.set("list", i);*/
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
