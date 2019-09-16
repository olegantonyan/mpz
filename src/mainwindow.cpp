#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowTitle("mpz");
  library = new DirectoryTreeViewModel(ui->treeView, "/mnt/storage/music", this);
  playlists = new PlaylistsViewModel(ui->listView, this);
  connect(library, &DirectoryTreeViewModel::createNewPlaylist, playlists, &PlaylistsViewModel::on_createPlaylist);
}

MainWindow::~MainWindow() {
  delete ui;
}
