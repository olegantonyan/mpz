#include "playlistitem.h"

#include <QDebug>
#include <QDir>

namespace Playlists {
  PlaylistItem::PlaylistItem(const QDir &path) {
    this->path = path;
    setName(path.dirName());

    qDebug() << QDir(path);
  }

  QDir PlaylistItem::getPath() const {
    return path;
  }
  
  QString PlaylistItem::getName() const {
    return name;
  }
  
  void PlaylistItem::setName(const QString &value) {
    name = value;
  }
}
