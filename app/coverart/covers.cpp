#include "coverart/covers.h"

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
      const auto local = bestLocalImage(key);
      if (!local.file.isEmpty() && local.score >= 0) {
        found = local.file;
        cache.insert(key, found);
      } else {
        // Embedded art is per file; the dir-keyed `cache` would serve it to the
        // whole directory, so it is deliberately left uncached.
        found = embedded_covers.get(filepath);
        if (found.isEmpty() && !local.file.isEmpty()) {
          found = local.file;
          cache.insert(key, found);
        }
      }
    }

    if (found.isEmpty()) {
      // Downloaded covers are keyed by artist+album, so for the same reason as
      // above they must not go into the dir-keyed `cache` either. Cheap enough
      // to re-check: a hash plus a few QFile::exists.
      found = Online::Cache::instance().lookup(Online::AlbumQuery{artist, album});
    }

    return found;
  }

  QString Covers::keyByFilepath(const QString &filepath) const {
    return QFileInfo(filepath).absoluteDir().absolutePath();
  }

  FolderCover::Match Covers::bestLocalImage(const QString &dir) const {
    const auto top = bestImageIn(dir);
    if (!top.file.isEmpty()) {
      return top;
    }

    // Descend one level only when the album folder itself holds no image.
    FolderCover::Match sub;
    QDirIterator dirs(dir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
    while (dirs.hasNext()) {
      dirs.next();
      const auto candidate = bestImageIn(dirs.filePath());
      if (!candidate.file.isEmpty() && (sub.file.isEmpty() || candidate.score > sub.score)) {
        sub = candidate;
      }
    }
    return sub;
  }
}
