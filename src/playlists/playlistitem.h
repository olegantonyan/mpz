#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QString>
#include <QDir>

namespace Playlists {
  class PlaylistItem {
  public:
    explicit PlaylistItem(const QDir &path);

    QDir getPath() const;

    QString getName() const;
    void setName(const QString &value);

  private:
    QDir path;
    QString name;
  };
}
#endif // PLAYLISTITEM_H
