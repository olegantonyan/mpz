#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "config/storage.h"

#include <QDebug>
#include <QApplication>
#include <memory>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowTitle("mpz");

  library = new DirectoryUi::View(ui->treeView, "/mnt/storage/music", this);
  playlists = new PlaylistsUi::View(ui->listView, this);
  playlist = new PlaylistUi::View(ui->tableView, this);

  connect(library, &DirectoryUi::View::createNewPlaylist, playlists, &PlaylistsUi::View::on_createPlaylist);
  connect(playlists, &PlaylistsUi::View::selected, playlist, &PlaylistUi::View::on_load);
  connect(playlists, &PlaylistsUi::View::emptied, playlist, &PlaylistUi::View::on_unload);

  loadUiSettings();

 /* qDebug() << qRegisterMetaType<QList<int>>("QList<int>");
  QList<int> list{1,2,3};
  QVariant v = QVariant::fromValue<QList<int>>(list);
  qDebug() << v.userType();*/
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::loadUiSettings() {
  restoreGeometry(local_conf.windowGeomentry());
  restoreState(local_conf.windowState());

  /*connect(ui->splitter, &QSplitter::splitterMoved, [=](int pos, int index) {
    (void)pos;
    (void)index;
    settings->setValue("splitter_size_1", ui->splitter->sizes().at(0));
    settings->setValue("splitter_size_2", ui->splitter->sizes().at(1));
    settings->setValue("splitter_size_3", ui->splitter->sizes().at(2));
  });

  auto i = QList<int>();
  bool all_ok = true;
  bool ok = false;
  i << settings->value("splitter_size_1").toInt(&ok);
  if (!ok) {
    all_ok = false;
  }
  i << settings->value("splitter_size_2").toInt(&ok);
  if (!ok) {
    all_ok = false;
  }
  i << settings->value("splitter_size_3").toInt(&ok);
  if (!ok) {
    all_ok = false;
  }

  if (all_ok) {
    ui->splitter->setSizes(i);
  }*/
}

void MainWindow::closeEvent(QCloseEvent *event) {
  local_conf.saveWindowGeometry(saveGeometry());
  local_conf.saveWindowState(saveState());
  local_conf.sync();
  QMainWindow::closeEvent(event);

}
