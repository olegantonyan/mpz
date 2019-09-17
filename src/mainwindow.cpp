#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowTitle("mpz");
  library = new Directory::DirectoryViewModel(ui->treeView, "/mnt/storage/music", this);
  playlists = new Playlists::PlaylistsViewModel(ui->listView, this);
  connect(library, &Directory::DirectoryViewModel::createNewPlaylist, playlists, &Playlists::PlaylistsViewModel::on_createPlaylist);
}

MainWindow::~MainWindow() {
  delete ui;
}
