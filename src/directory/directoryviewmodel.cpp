#include "directoryviewmodel.h"

#include <QAction>
#include <QDebug>
#include <QMenu>

namespace Directory {
  DirectoryViewModel::DirectoryViewModel(QTreeView *v, const QString &library_path, QObject *parent) : QObject(parent) {
    view = v;

    model = new Directory::DirectoryDataModel(library_path, this);

    view->setModel(model);
    view->setRootIndex(model->index(library_path));
    view->setHeaderHidden(true);
    view->setColumnHidden(1, true);
    view->setColumnHidden(2, true);
    view->setColumnHidden(3, true);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(view, &QTreeView::customContextMenuRequested, this, &DirectoryViewModel::on_customContextMenuRequested);
  }

  void DirectoryViewModel::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);
    auto filepath = model->filePath(index);

    QMenu menu;
    QAction create_playlist("Create new playlist");
    QAction append_to_playlist("Append to current playlist");

    connect(&create_playlist, &QAction::triggered, [=]() { emit createNewPlaylist(filepath); });
    connect(&append_to_playlist, &QAction::triggered, [=]() { emit appendToCurrentPlaylist(filepath); });

    menu.addAction(&create_playlist);
    menu.addAction(&append_to_playlist);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
