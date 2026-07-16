#include "coverart/online/cache.h"
#include "lyrics/textmatch.h"

namespace CoverArt {
  namespace Online {
    namespace {
      const QStringList IMAGE_EXTENSIONS = {"jpg", "jpeg", "png", "webp", "gif", "bmp"};
      const int MAX_CACHED_COVERS = 2000;
    }

    Cache &Cache::instance() {
      static Cache self;
      return self;
    }

    Cache::Cache() : store("downloaded_covers") {
    }

    QString Cache::key(const AlbumQuery &query) {
      return DiskCache::Store::hash(Lyrics::TextMatch::normalizeArtist(query.artist),
                                    Lyrics::TextMatch::normalizeTitle(query.album));
    }

    QString Cache::lookup(const AlbumQuery &query) const {
      return store.find(key(query), IMAGE_EXTENSIONS);
    }

    bool Cache::isKnownMiss(const AlbumQuery &query, const QStringList &providers) const {
      return store.isKnownMiss(key(query), providers);
    }

    QString Cache::storeFound(const AlbumQuery &query, const QByteArray &image, const QString &format) {
      QString ext = format.toLower();
      if (!IMAGE_EXTENSIONS.contains(ext)) {
        ext = "jpg";
      }
      // Bytes go in verbatim, unlike CoverArt::Embedded which re-encodes to PNG.
      const QString path = store.write(key(query), ext, image);
      if (!path.isEmpty()) {
        store.trim(IMAGE_EXTENSIONS, MAX_CACHED_COVERS);
      }
      return path;
    }

    void Cache::storeNotFound(const AlbumQuery &query, const QStringList &providers) {
      store.storeMiss(key(query), providers);
    }
  }
}
