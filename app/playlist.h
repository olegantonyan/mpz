#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include "track.h"

#include <QString>
#include <QDir>
#include <QVector>

class Playlist {
public:
  explicit Playlist(const QDir &path);

  QDir path() const;

  QString name() const;
  QString rename(const QString &value);
  QVector<Track> tracks() const;

private:
  QDir directory_path;
  QString playlist_name;
};

#endif // PLAYLISTITEM_H
