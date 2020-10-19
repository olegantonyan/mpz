#include "playlist/playlist.h"
#include "playlist/fileparser.h"
#include "playlist/cueparser.h"
#include "playlist/loader.h"
#include "rnjesus.h"

#include <QDebug>
#include <QDirIterator>
#include <QtConcurrent>
#include <QFileInfo>
#include <QMutableVectorIterator>

namespace Playlist {
  Playlist::Playlist() : QObject(nullptr) {
    _uid = RNJesus::generate();
    _random = PlaylistRandom::None;
  }

  QString Playlist::name() const {
    return playlist_name;
  }

  QString Playlist::rename(const QString &value) {
    const int MAX_NAME_LEN = 69;
    if (value.length() > MAX_NAME_LEN) {
      playlist_name = value.left(MAX_NAME_LEN - 3) + "...";
    } else {
      playlist_name = value;
    }
    return playlist_name;
  }

  QVector<Track> Playlist::tracks() const {
    return tracks_list;
  }

  bool Playlist::load(const QDir &path) {
    rename(nameBy(path));
    concat(path);
    return true;
  }

  void Playlist::loadAsync(const QList<QDir> &dirs) {
    QtConcurrent::run([=]() {
      load(dirs);
      emit loadAsyncFinished(this);
    });
  }

  bool Playlist::load(const QVector<Track> &tracks) {
    tracks_list.append(tracks);
    return true;
  }

  bool Playlist::load(const QList<QDir> &dirs) {
    bool ok = true;
    for (auto i : dirs) {
      if (!load(i)) {
        ok = false;
      }
    }
    QStringList lst;
    for (auto i : dirs) { lst << nameBy(i); }
    rename(lst.join(", "));
    return ok;
  }

  bool Playlist::concat(const QDir &path) {
    tracks_list.append(sort(Loader(path).tracks()));
    return true;
  }

  bool Playlist::concat(const QList<QDir> &dirs) {
    bool ok = true;
    for (auto i : dirs) {
      if (!concat(i)) {
        ok = false;
      }
    }
    return ok;
  }

  void Playlist::concatAsync(const QList<QDir> &dirs) {
    QtConcurrent::run([=]() {
      concat(dirs);
      emit concatAsyncFinished(this);
    });
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

  QVector<Track> Playlist::sort(QVector<Track> list, const Sorter &sorter) {
    std::sort(list.begin(), list.end(), [&](const Track &t1, const Track &t2) -> bool {
      return sorter.condition(t1, t2);
    });
    return list;
  }

  void Playlist::removeTrack(int position) {
    tracks_list.remove(position);
  }

  Playlist::PlaylistRandom Playlist::random() const {
    return _random;
  }

  void Playlist::setRandom(Playlist::PlaylistRandom arg) {
    _random = arg;
  }

  void Playlist::sortBy(const QString &criteria) {
    tracks_list = sort(tracks_list, Sorter(criteria));
  }

  QString Playlist::nameBy(const QDir &path) {
    return path.dirName();
  }
}

