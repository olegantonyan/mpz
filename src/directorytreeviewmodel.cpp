#include "directorytreeviewmodel.h"

#include <QAction>
#include <QDebug>

DirectoryTreeViewModel::DirectoryTreeViewModel(QTreeView *v, const QString &library_path, QObject *parent) : QObject(parent) {
  view = v;
  context_menu = new QMenu(view);

  context_menu->addAction(new QAction("Action 1", view));
  context_menu->addAction(new QAction("Action 2", view));
  context_menu->addAction(new QAction("Action 3", view));

  model = new QFileSystemModel(this);
  model->setReadOnly(true);
  model->setRootPath(library_path);

  view->setModel(model);
  view->setRootIndex(model->index(library_path));
  view->setHeaderHidden(true);
  view->setColumnHidden(1, true);
  view->setColumnHidden(2, true);
  view->setColumnHidden(3, true);
  view->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(view, &QTreeView::customContextMenuRequested, this, &DirectoryTreeViewModel::on_customContextMenuRequested);
}

void DirectoryTreeViewModel::on_customContextMenuRequested(const QPoint &pos) {
  auto index = view->indexAt(pos);

  qDebug() << model->filePath(index);
  qDebug() << model->fileName(index);

  context_menu->popup(view->viewport()->mapToGlobal(pos));
}
