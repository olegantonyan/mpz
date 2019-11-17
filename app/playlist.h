#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include "track.h"

#include <QString>
#include <QDir>
#include <QVector>

class Playlist {
public:
  Playlist();

  QString name() const;
  QString rename(const QString &value);
  QVector<Track> tracks() const;

  bool load(const QDir &path);
  bool load(const QVector<Track> &tracks);

  quint64 uid() const;

private:
  QString playlist_name;
  QVector<Track> tracks_list;
  quint64 _uid;
};

#endif // PLAYLISTITEM_H
