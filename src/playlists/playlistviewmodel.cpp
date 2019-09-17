#include "playlistsviewmodel.h"

#include <QDebug>

namespace Playlists {
  PlaylistsViewModel::PlaylistsViewModel(QListView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new QStringListModel(this);

    view->setModel(model);
  }

  void PlaylistsViewModel::on_createPlaylist(const QString &filepath) {
    qDebug() << filepath;
    auto list = model->stringList();
    list << filepath;
    model->setStringList(list);
  }
}
