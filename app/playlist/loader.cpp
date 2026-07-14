#include "loader.h"
#include "playlist/fileparser.h"
#include "playlist/cueparser.h"

#include <QAtomicInt>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureSynchronizer>
#include <QSet>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent>

#include <utility>

namespace Playlist {
  namespace {
    QString canonical_or_abs(const QString& p) {
      QString c = QFileInfo(p).canonicalFilePath();
      return c.isEmpty() ? QFileInfo(p).absoluteFilePath() : c;
    }

    QThreadPool &scan_pool() {
      // Deliberately not globalInstance(): tracks() itself runs on a global-pool thread
      // and blocks here, so sharing that pool could starve it.
      static QThreadPool pool;
      return pool;
    }

    // Applies fn to every path concurrently, preserving input order.
    template <typename T, typename F>
    QVector<T> parallel_map(const QStringList &paths, F fn) {
      QVector<T> out(paths.size());
      if (paths.isEmpty()) {
        return out;
      }

      T *out_data = out.data(); // detach once here so workers never race on the refcount
      QAtomicInt next(0);
      const int workers = qBound(1, QThread::idealThreadCount(), paths.size());

      QFutureSynchronizer<void> sync;
      for (int w = 0; w < workers; ++w) {
        sync.addFuture(QtConcurrent::run(&scan_pool(), [&paths, &next, out_data, fn]() {
          for (int i = next.fetchAndAddOrdered(1); i < paths.size(); i = next.fetchAndAddOrdered(1)) {
            out_data[i] = fn(paths.at(i));
          }
        }));
      }
      sync.waitForFinished();

      return out;
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

    QStringList audio_paths;
    QStringList cue_paths;
    QDirIterator it(path.absolutePath(), files_filter(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      const auto current_path = it.next();
      (is_cue(current_path) ? cue_paths : audio_paths) << current_path;
    }

    QVector<Track> result;

    // Audio file paths (canonicalized) referenced by any cue. Files a cue already
    // covers are never tag-read as standalone tracks below.
    QSet<QString> cue_audio_paths;
    // (canonical_audio_path + "@" + begin_ms) -- used to drop duplicate cue
    // entries when more than one cue covers the same audio at the same offset.
    QSet<QString> seen_cue_keys;

    const auto per_cue = parallel_map<QVector<Track>>(cue_paths, [](const QString &p) {
      return CueParser(p).tracks_list();
    });
    for (const auto &current_cues : per_cue) {
      for (const auto &c : current_cues) {
        const QString canon = canonical_or_abs(c.path());
        cue_audio_paths.insert(canon);
        const QString key = canon + QChar('@') + QString::number(c.begin());
        if (seen_cue_keys.contains(key)) {
          continue; // duplicate cue entry from a sibling cue
        }
        seen_cue_keys.insert(key);
        result.append(c);
      }
    }

    QStringList to_read;
    if (cue_audio_paths.isEmpty()) {
      to_read = audio_paths;
    } else {
      for (const auto &p : std::as_const(audio_paths)) {
        if (!cue_audio_paths.contains(canonical_or_abs(p))) {
          to_read << p;
        }
      }
    }

    result.append(parallel_map<Track>(to_read, [](const QString &p) {
      return Track(p);
    }));

    return result;
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
