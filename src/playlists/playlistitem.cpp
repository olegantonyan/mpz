#include "playlistitem.h"

namespace Playlists {
  PlaylistItem::PlaylistItem(const QString &path) {
    this->path = path;
  }

  QString PlaylistItem::getPath() const {
    return path;
  }
}
