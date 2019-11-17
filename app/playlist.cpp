#include "playlist.h"

#include <QDebug>
#include <QDirIterator>
#include <QRandomGenerator>

Playlist::Playlist() {
  _uid = QRandomGenerator::global()->generate64();
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

bool Playlist::load(const QDir &path) {
  rename(path.dirName());
  QDirIterator it(path.absolutePath(), QStringList() << "*.mp3" << "*.flac" << "*.ogg", QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    tracks_list << Track(it.next());
  }
  return true;
}

bool Playlist::load(const QVector<Track> &tracks) {
  tracks_list = tracks;
  return true;
}

quint64 Playlist::uid() const {
  return _uid;
}

