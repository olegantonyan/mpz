#include "playlistsviewmodel.h"
#include "playlistitem.h"

#include <QDebug>

namespace Playlists {
  PlaylistsViewModel::PlaylistsViewModel(QListView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Playlists::PlaylistsDataModel(this);

    view->setModel(model);
  }

  void PlaylistsViewModel::on_createPlaylist(const QString &filepath) {
    qDebug() << filepath;
    model->append(Playlists::PlaylistItem(filepath));
    /*auto list = model->stringList();
    list << filepath;
    model->setStringList(list);*/
  }
}
