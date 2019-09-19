#include "playlist.h"

#include <QDebug>

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

