#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QString>

namespace Playlists {
  class PlaylistItem {
  public:
    PlaylistItem(const QString &path);

    QString getPath() const;

  private:
    QString path;
  };
}
#endif // PLAYLISTITEM_H
