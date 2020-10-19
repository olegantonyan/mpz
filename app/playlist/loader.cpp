#include "loader.h"
#include "playlist/fileparser.h"
#include "playlist/cueparser.h"

#include <QDirIterator>

namespace Playlist {
  Loader::Loader(const QDir &p) : path(p) {
  }

  QStringList Loader::supportedFileFormats() {
    return QStringList() << "mp3" << "flac" << "ogg" << "m4a" << "mp4" << "wav" << "wma" << "aac" << "ape" << "cue" << "opus" << "dsf";
  }

  QStringList Loader::supportedPlaylistFileFormats() {
    return QStringList() << "m3u" << "pls";
  }

  QVector<Track> Loader::tracks() const {
    if (is_playlist_file()) {
      return FileParser(path).tracks_list();
    }
    if (is_single_file()) {
      return QVector<Track>() << Track(path.absolutePath());
    }

    QVector<Track> result;

    QStringList cues;
    QDirIterator it(path.absolutePath(), files_filter(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      auto current_path = it.next();
      if (is_cue(current_path)) {
        auto current_cues = CueParser(current_path).tracks_list();
        if (!current_cues.isEmpty()) {
          cues.append(current_cues.first().path());
          result.append(current_cues);
        }
      } else {
        result.append(Track(current_path));
      }
    }

    if (!cues.isEmpty()) { // remove files that are already added from CUE
      QMutableVectorIterator<Track> mit(result);
      while (mit.hasNext()) {
        auto track = mit.next();
        if (!track.isCue() && cues.contains(track.path())) {
          mit.remove();
        }
      }
    }

    return result;
  }

  bool Loader::is_file() const {
    for (auto i : Loader::supportedFileFormats()) {
      if (path.dirName().endsWith(i, Qt::CaseInsensitive)) {
        return true;
      }
    }
    return false;
  }

  bool Loader::is_dir_empty() const {
  #if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    return path.isEmpty();
  #else
    return path.count() == 0;
#endif
  }

  bool Loader::is_single_file() const {
    return is_dir_empty() && is_file();
  }

  bool Loader::is_playlist_file() const {
    for (auto i : Loader::supportedPlaylistFileFormats()) {
      if (path.dirName().endsWith(i, Qt::CaseInsensitive)) {
        return true;
      }
    }
    return false;
  }

  QStringList Loader::files_filter() const {
    QStringList filter;
    for (auto i : Loader::supportedFileFormats()) {
      filter << QString("*.") + i;
    }
    return filter;
  }

  bool Loader::is_cue(const QString &current_path) const {
    return current_path.endsWith(".cue", Qt::CaseInsensitive);
  }
}
