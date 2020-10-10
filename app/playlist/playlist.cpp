#include "playlist/playlist.h"
#include "playlist/fileparser.h"
#include "playlist/cueparser.h"
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

  QStringList Playlist::supportedFileFormats() {
    return QStringList() << "mp3" << "flac" << "ogg" << "m4a" << "mp4" << "wav" << "wma" << "aac" << "ape" << "cue" << "opus";
  }

  QStringList Playlist::supportedPlaylistFileFormats() {
    return QStringList() << "m3u" << "pls";
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

    for (auto i : Playlist::supportedPlaylistFileFormats()) {
      if (path.dirName().endsWith(i, Qt::CaseInsensitive)) {
        tracks_list = FileParser(path).tracks_list();
        return true;
      }
    }

    bool is_file = false;
    for (auto i : Playlist::supportedFileFormats()) {
      if (path.dirName().endsWith(i, Qt::CaseInsensitive)) {
        is_file = true;
      }
    }

    #if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
      bool empty = path.isEmpty();
    #else
      bool empty = path.count() == 0;
    #endif
    if (empty && is_file) { // playlist file
      tracks_list << Track(path.absolutePath());
    } else {
      QStringList filter;
      for (auto i : Playlist::supportedFileFormats()) {
        filter << QString("*.") + i;
      }
      QStringList cues;
      QDirIterator it(path.absolutePath(), filter, QDir::Files, QDirIterator::Subdirectories);
      while (it.hasNext()) {
        auto path = it.next();
        if (path.endsWith(".cue", Qt::CaseInsensitive)) {
          auto current_cues = CueParser(path).tracks_list();
          if (!current_cues.isEmpty()) {
            cues.append(current_cues.first().path());
            tracks_list.append(current_cues);
          }
        } else {
          tracks_list.append(Track(path));
        }
      }

      QMutableVectorIterator<Track> mit(tracks_list);
      while (mit.hasNext()) {
        auto track = mit.next();
        if (!track.isCue() && cues.contains(track.path())) {
          mit.remove();
        }
      }

      sort();
    }
    return true;
  }

  void Playlist::loadAsync(const QDir &path) {
    QtConcurrent::run([=]() {
      load(path);
      emit loadAsyncFinished(this);
    });
  }

  bool Playlist::load(const QVector<Track> &tracks) {
    for (auto i : tracks) {
      //if (i.isValid()) { // load all regardless to prevent saving empty playlists (#65)
        tracks_list << i;
      //}
    }
    return true;
  }

  bool Playlist::concat(const QDir &path) {
    Playlist new_playlist;
    new_playlist.load(path);
    for (auto i : new_playlist.tracks()) {
      tracks_list << i;
    }
    return true;
  }

  void Playlist::concatAsync(const QDir &path) {
    QtConcurrent::run([=]() {
      concat(path);
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

  void Playlist::sort() {
     // %ARTIST% - %DATE% - %ALBUM% - %DISCNUMBER% - %TRACKNUMBER% - %TITLE%
    std::sort(tracks_list.begin(), tracks_list.end(), [](const Track &t1, const Track &t2) -> bool {
      /*if (t1.artist() < t2.artist()) {
        return true;
      } else if (t1.artist() > t2.artist()) {
        return false;
      }*/

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

  void Playlist::removeTrack(int position) {
    tracks_list.remove(position);
  }

  Playlist::PlaylistRandom Playlist::random() const {
    return _random;
  }

  void Playlist::setRandom(Playlist::PlaylistRandom arg) {
    _random = arg;
  }
}

