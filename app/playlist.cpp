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
  sort();
  return true;
}

bool Playlist::load(const QVector<Track> &tracks) {
  tracks_list = tracks;
  return true;
}

bool Playlist::concat(const QDir &path) {
  auto new_playlist = Playlist();
  new_playlist.load(path);
  for (auto i : new_playlist.tracks()) {
    tracks_list << i;
  }
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

void Playlist::sort() {
   // %ARTIST% - %DATE% - %ALBUM% - %DISCNUMBER% - %TRACKNUMBER% - %TITLE%
  std::sort(tracks_list.begin(), tracks_list.end(), [](const Track &t1, const Track &t2) -> bool {
    if (t1.artist() < t2.artist()) {
      return true;
    } else if (t1.artist() > t2.artist()) {
      return false;
    }

    if (t1.year() < t2.year()) {
      return true;
    } else if (t1.year() > t2.year()) {
      return false;
    }

    if (t1.album() < t2.album()) {
      return true;
    } else if (t1.album() > t2.album()) {
      return false;
    }

    if (t1.track_number() < t2.track_number()) {
      return true;
    } else if (t1.track_number() > t2.track_number()) {
      return false;
    }

    if (t1.filename() < t2.filename()) {
      return true;
    } else if (t1.filename() > t2.filename()) {
      return false;
    }

    if (t1.title() < t2.title()) {
      return true;
    } else if (t1.title() > t2.title()) {
      return false;
    }

    return false;
  });
}

