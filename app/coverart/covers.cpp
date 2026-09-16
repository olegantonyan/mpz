#include "coverart/covers.h"
#include "playlist/loader.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QBuffer>

namespace CoverArt {
  static Covers *self = nullptr;

  static FolderCover::Match bestImageIn(const QString &dir) {
    static const QStringList nmask{"*.jpg", "*.jpeg", "*.png", "*.webp",
                                   "*.gif", "*.tiff", "*.bmp"};
    QStringList names;
    QDirIterator it(dir, nmask, QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
      it.next();
      names << it.fileName();
    }
    auto match = FolderCover::best(names);
    if (!match.file.isEmpty()) {
      match.file = QDir(dir).absoluteFilePath(match.file);
    }
    return match;
  }

  static bool holdsAudio(const QString &dir) {
    static const QStringList nmask = [] {
      QStringList mask;
      for (const auto &ext : Playlist::Loader::supportedFileFormats()) {
        mask << QStringLiteral("*.") + ext;
      }
      return mask;
    }();
    QDirIterator it(dir, nmask, QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext();
  }

  static FolderCover::Match bestArtworkSubfolderImage(const QString &dir) {
    FolderCover::Match result;
    QDirIterator dirs(dir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
    while (dirs.hasNext()) {
      dirs.next();
      const auto candidate = bestImageIn(dirs.filePath());
      if (candidate.file.isEmpty() || (!result.file.isEmpty() && candidate.score <= result.score)) {
        continue;
      }
      if (!holdsAudio(dirs.filePath())) {
        result = candidate;
      }
    }
    return result;
  }

  Covers &Covers::instance(ModusOperandi &modus) {
    if (self == nullptr) {
      self = new Covers(modus);
    }
    return *self;
  }

  Covers &Covers::instance() {
    if (!self) {
      qFatal("Covers::instance() used before initialization");
    }
    return *self;
  }

  Covers::Covers(ModusOperandi &modus) :
    modus_operandi(modus)
#ifdef ENABLE_MPD_SUPPORT
    , mpd_covers(modus.mpd_client)
#endif
  {
  }

  QString Covers::get(const QString &filepath, const QString &artist, const QString &album) {
    if (filepath.isEmpty()) {
      return QString();
    }

    auto key = keyByFilepath(filepath);
    if (cache.contains(key)) {
      return cache.value(key);
    }

    QString found;
    if (modus_operandi.get() == ModusOperandi::MODUS_MPD) {
#ifdef ENABLE_MPD_SUPPORT
      found = mpd_covers.get(filepath);
      if (!found.isEmpty()) {
        cache.insert(key, found);
      }
#endif
    } else if (modus_operandi.get() == ModusOperandi::MODUS_LOCALFS) {
      const auto own = bestImageIn(key);
      if (!own.file.isEmpty() && own.score >= 0) {
        found = own.file;
        cache.insert(key, found);
      } else {
        // Embedded art is per file, so neither it nor the fallbacks below it may go into the dir-keyed `cache`.
        found = embedded_covers.get(filepath);
        if (found.isEmpty()) {
          found = own.file.isEmpty() ? bestArtworkSubfolderImage(key).file : own.file;
        }
      }
    }

    if (found.isEmpty()) {
      // Downloaded covers are keyed by artist+album, so for the same reason as above they must not go
      // into the dir-keyed `cache` either. Cheap enough to re-check: a hash plus a few QFile::exists.
      found = Online::Cache::instance().lookup(Online::AlbumQuery{artist, album});
    }

    return found;
  }

  QString Covers::keyByFilepath(const QString &filepath) const {
    return QFileInfo(filepath).absoluteDir().absolutePath();
  }
}
