#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  const auto library_path = "/mnt/storage/music";

  treeViewModel = new QFileSystemModel(this);
  treeViewModel->setReadOnly(true);
  treeViewModel->setRootPath(library_path);

  ui->treeView->setModel(treeViewModel);
  ui->treeView->setRootIndex(treeViewModel->index(library_path));
  ui->treeView->setHeaderHidden(true);
  ui->treeView->setColumnHidden(1, true);
  ui->treeView->setColumnHidden(2, true);
  ui->treeView->setColumnHidden(3, true);
  ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
}

MainWindow::~MainWindow() {
  delete ui;
}


void MainWindow::on_treeView_clicked(const QModelIndex &index) {
  //qDebug() << treeViewModel->filePath(index);
}

void MainWindow::on_treeView_customContextMenuRequested(const QPoint &pos) {
  auto index = ui->treeView->indexAt(pos);

  qDebug() << treeViewModel->filePath(index);
  qDebug() << treeViewModel->fileName(index);


  auto *menu = new QMenu(this);
  menu->addAction(new QAction("Action 1", this));
  menu->addAction(new QAction("Action 2", this));
  menu->addAction(new QAction("Action 3", this));
  menu->popup(ui->treeView->viewport()->mapToGlobal(pos));

  //qDebug() << pos;
}
