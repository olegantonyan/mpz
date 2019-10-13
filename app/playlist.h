#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include "track.h"

#include <QString>
#include <QDir>
#include <QVector>

class Playlist {
public:
  explicit Playlist();

  QString name() const;
  QString rename(const QString &value);
  QVector<Track> tracks() const;

  bool load(const QDir &path);
  bool load(const QVector<Track> &tracks);

private:
  QString playlist_name;
  QVector<Track> tracks_list;
};

#endif // PLAYLISTITEM_H
