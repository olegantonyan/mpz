#include "loader.h"
#include "playlist/fileparser.h"
#include "playlist/cueparser.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

namespace Playlist {
  namespace {
    QString canonical_or_abs(const QString& p) {
      QString c = QFileInfo(p).canonicalFilePath();
      return c.isEmpty() ? QFileInfo(p).absoluteFilePath() : c;
    }
  }

  Loader::Loader(const QDir &p) : path(p) {
  }

  const QStringList &Loader::supportedFileFormats() {
    static const QStringList formats = {
      QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("ogg"),
      QStringLiteral("m4a"), QStringLiteral("mp4"), QStringLiteral("wav"),
      QStringLiteral("wma"), QStringLiteral("aac"), QStringLiteral("ape"),
      QStringLiteral("cue"), QStringLiteral("opus"), QStringLiteral("dsf")
    };
    return formats;
  }

  const QStringList &Loader::supportedPlaylistFileFormats() {
    static const QStringList formats = {
      QStringLiteral("m3u"), QStringLiteral("m3u8"), QStringLiteral("pls")
    };
    return formats;
  }

  bool Loader::is_supported_file(const QString &name) {
    for (const auto &i : Loader::supportedFileFormats()) {
      if (name.endsWith(i, Qt::CaseInsensitive)) {
        return true;
      }
    }
    return false;
  }

  QVector<Track> Loader::tracks() const {
    if (is_playlist_file()) {
      return FileParser(path).tracks_list();
    }
    if (is_single_file()) {
      return QVector<Track>() << Track(path.absolutePath());
    }

    QVector<Track> result;

    // Audio file paths (canonicalized) referenced by any cue we encounter.
    // Used to drop the "raw" Track entries for files that a cue already covers.
    QSet<QString> cue_audio_paths;
    // (canonical_audio_path + "@" + begin_ms) -- used to drop duplicate cue
    // entries when more than one cue covers the same audio at the same offset.
    QSet<QString> seen_cue_keys;

    QDirIterator it(path.absolutePath(), files_filter(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      auto current_path = it.next();
      if (is_cue(current_path)) {
        auto current_cues = CueParser(current_path).tracks_list();
        for (const auto& c : current_cues) {
          const QString canon = canonical_or_abs(c.path());
          cue_audio_paths.insert(canon);
          const QString key = canon + QChar('@') + QString::number(c.begin());
          if (seen_cue_keys.contains(key)) {
            continue; // duplicate cue entry from a sibling cue
          }
          seen_cue_keys.insert(key);
          result.append(c);
        }
      } else {
        result.append(Track(current_path));
      }
    }

    remove_tracks_added_from_cue(cue_audio_paths, result);

    return result;
  }

  void Loader::remove_tracks_added_from_cue(const QSet<QString> &cue_audio_paths, QVector<Track> &tracks) const {
    if (cue_audio_paths.isEmpty()) {
      return;
    }
    QMutableVectorIterator<Track> mit(tracks);
    while (mit.hasNext()) {
      const auto& track = mit.next();
      if (track.isCue()) {
        continue;
      }
      if (cue_audio_paths.contains(canonical_or_abs(track.path()))) {
        mit.remove();
      }
    }
  }

  bool Loader::is_dir_empty() const {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    return path.isEmpty();
#else
    return path.count() == 0;
#endif
  }

  bool Loader::is_single_file() const {
    return is_dir_empty() && Loader::is_supported_file(path.dirName());
  }

  bool Loader::is_playlist_file() const {
    for (const auto &i : Loader::supportedPlaylistFileFormats()) {
      if (path.dirName().endsWith(i, Qt::CaseInsensitive)) {
        return true;
      }
    }
    return false;
  }

  QStringList Loader::files_filter() const {
    QStringList filter;
    for (const auto &i : Loader::supportedFileFormats()) {
      filter << QString("*.") + i;
    }
    return filter;
  }

  bool Loader::is_cue(const QString &current_path) const {
    return current_path.endsWith(".cue", Qt::CaseInsensitive);
  }
}
