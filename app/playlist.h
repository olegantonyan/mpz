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

  bool hasTrack(quint64 track_uid) const;
  int trackIndex(quint64 track_uid) const;
  Track trackBy(quint64 uid) const;

  void sort();

private:
  QString playlist_name;
  QVector<Track> tracks_list;
  quint64 _uid;
};

#endif // PLAYLISTITEM_H
