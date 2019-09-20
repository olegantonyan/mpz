#include "playlist.h"

#include <QDebug>
#include <QDirIterator>

Playlist::Playlist(const QDir &path) {
  directory_path = path;
  rename(path.dirName());
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
  QVector<Track> list;
  QDirIterator it(path().absolutePath(), QStringList() << "*.mp3" << "*.flac" << "*.ogg", QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    list << Track(it.next());
  }

  return list;
}

