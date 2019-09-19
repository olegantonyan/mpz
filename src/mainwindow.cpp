#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowTitle("mpz");
  library = new Directory::View(ui->treeView, "/mnt/storage/music", this);
  playlists = new Playlists::View(ui->listView, this);
  connect(library, &Directory::View::createNewPlaylist, playlists, &Playlists::View::on_createPlaylist);
}

MainWindow::~MainWindow() {
  delete ui;
}
