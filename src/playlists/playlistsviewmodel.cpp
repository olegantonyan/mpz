#include "playlistsviewmodel.h"
#include "playlistitem.h"

#include <QDebug>
#include <QMenu>
#include <QAction>

namespace Playlists {
  PlaylistsViewModel::PlaylistsViewModel(QListView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Playlists::PlaylistsDataModel(this);

    view->setModel(model);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(view, &QListView::customContextMenuRequested, this, &PlaylistsViewModel::on_customContextMenuRequested);
  }

  void PlaylistsViewModel::on_createPlaylist(const QDir &filepath) {
    qDebug() << "add directory to playlist" << filepath;
    model->append(Playlists::PlaylistItem(filepath));
  }

  void PlaylistsViewModel::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);

    QMenu menu;
    QAction remove("Remove");
    QAction rename("Rename");

    connect(&remove, &QAction::triggered, [=]() { model->remove(index); });
    connect(&rename, &QAction::triggered, [=]() {
      PlaylistItem i = model->itemAt(index);
      i.setName("ololo");
      model->repalceAt(index, i);
    });

    menu.addAction(&remove);
    menu.addAction(&rename);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
