#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  library = new DirectoryTreeViewModel(ui->treeView, "/mnt/storage/music", this);
}

MainWindow::~MainWindow() {
  delete ui;
}
