#include "coverart/covers.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QBuffer>

namespace CoverArt {
  static Covers *self = nullptr;

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
      found = findCoverLocally(key);
      if (found.isEmpty()) {
        // Not cached: `cache` is keyed by directory but embedded art is per
        // file, so caching it here would serve one track's art to the whole dir.
        found = embedded_covers.get(filepath);
      } else {
        cache.insert(key, found);
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

  QString Covers::findCoverLocally(const QString &dir) {
    const auto nmask = QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.gif" << "*.tiff" << "*.bmp";

    QString result;
    QDirIterator it(dir, nmask, QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
      auto current_file = it.next();
      if (result.isEmpty()) {
        result = current_file; // fallback to first image file found in the dir
      }

      if (current_file.contains("cover", Qt::CaseInsensitive)) {
        result = current_file;
        break;
      }
    }
    return result;
  }
}
