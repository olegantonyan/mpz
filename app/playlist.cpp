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

bool Playlist::hasTrack(quint64 track_uid) const {
  return trackIndex(track_uid) >= 0;
}

int Playlist::trackIndex(quint64 track_uid) const {
  for (int i = 0; i < tracks().size(); i++) {
    if (tracks().at(i).uid() == track_uid) {
      return i;
    }
  }
  return -1;
}

Track Playlist::trackBy(quint64 uid) const {
  for (auto i : tracks()) {
    if (i.uid() == uid) {
      return i;
    }
  }
  return Track();
}

