#include "playlistsviewmodel.h"

#include <QDebug>

PlaylistsViewModel::PlaylistsViewModel(QListView *v, QObject *parent) : QObject(parent) {
  view = v;
}

void PlaylistsViewModel::on_createPlaylist(const QString &filepath) {
  qDebug() << filepath;
}
