#include "playlist.h"

#include <QDebug>
#include <QDirIterator>

Playlist::Playlist(const QDir &p) {
  directory_path = p;
  rename(path().dirName());
  tracks_list.clear();

  qDebug() << "initializing playlist" << path();

  // TODO do this in background
  QDirIterator it(path().absolutePath(), QStringList() << "*.mp3" << "*.flac" << "*.ogg", QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    tracks_list << Track(it.next());
  }
}

QDir Playlist::path() const {
  return directory_path;
}

QString Playlist::name() const {
  return playlist_name;
}

QString Playlist::rename(const QString &value) {
  playlist_name = value;
  return name();
}

QVector<Track> Playlist::tracks() const {
  return tracks_list;
}

