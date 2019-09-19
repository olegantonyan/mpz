#include "playlistsviewmodel.h"
#include "playlistitem.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>

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
      bool ok;
      QString new_name = QInputDialog::getText(view, QString("Rename playlist %1").arg(i.getName()), "", QLineEdit::Normal, i.getName(), &ok, Qt::Widget);
      if (ok && !new_name.isEmpty()) {
        i.setName(new_name);
        model->repalceAt(index, i);
      }
    });

    menu.addAction(&rename);
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
