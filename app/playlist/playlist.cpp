#include "playlist/playlist.h"
#include "playlist/loader.h"
#include "rnjesus.h"

#include <QDebug>
#include <QDirIterator>
#include <QtConcurrent>
#include <QFileInfo>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #include <QMutableListIterator>
#else
  #include <QMutableVectorIterator>
#endif
#include <QMutexLocker>

namespace Playlist {
  Playlist::Playlist() {
    _uid = RNJesus::generate();
    _random = PlaylistRandom::None;
  }

  QString Playlist::name() const {
    return playlist_name;
  }

  QString Playlist::rename(const QString &value) {
    QMutexLocker lock(&mutex);
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

  void Playlist::load(const QVector<Track> &tracks) {
    QMutexLocker lock(&mutex);
    tracks_list.clear();
    tracks_list.append(tracks);
  }

  void Playlist::append(const QVector<Track> &tracks, bool with_sort) {
    QMutexLocker lock(&mutex);
    if (with_sort) {
      tracks_list.append(sort(tracks));
    } else {
      tracks_list.append(tracks);
    }
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
    QMutexLocker lock(&mutex);
    tracks_list.remove(position);
  }

  Playlist::PlaylistRandom Playlist::random() const {
    return _random;
  }

  void Playlist::setRandom(Playlist::PlaylistRandom arg) {
    QMutexLocker lock(&mutex);
    _random = arg;
  }

  void Playlist::sortBy(const QString &criteria) {
    QMutexLocker lock(&mutex);
    tracks_list = sort(tracks_list, Sorter(criteria));
  }

  QByteArray Playlist::toM3U() const {
    QStringList result;
    for (auto i : tracks()) {
      if (i.isStream()) {
        result << i.url().toString();
      } else {
        result << i.path();
      } // TODO handle CUE maybe?
    }
    return result.join("\n").toUtf8();
  }

  void Playlist::reload() {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QMutableListIterator<Track> mit(tracks_list);
#else
    QMutableVectorIterator<Track> mit(tracks_list);
#endif
    while (mit.hasNext()) {
      mit.next().reload();
    }
  }
}
