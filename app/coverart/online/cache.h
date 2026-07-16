#ifndef COVERART_ONLINE_CACHE_H
#define COVERART_ONLINE_CACHE_H

#include "coverart/online/albumquery.h"
#include "diskcache.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace CoverArt {
  namespace Online {
    // Persistent on-disk cache of downloaded album covers, keyed by artist +
    // album. Survives restarts — it is the only cover storage that does.
    class Cache {
    public:
      static Cache &instance();

      QString dir() const { return store.dir(); }

      // Path of the cached cover, or empty if there isn't one.
      QString lookup(const AlbumQuery &query) const;
      // True when a previous search with this exact provider set came up empty
      // and the sentinel hasn't expired yet.
      bool isKnownMiss(const AlbumQuery &query, const QStringList &providers) const;

      QString storeFound(const AlbumQuery &query, const QByteArray &image, const QString &format);
      void storeNotFound(const AlbumQuery &query, const QStringList &providers);

      // Removes every cached cover and miss sentinel. Returns the file count.
      int clear() { return store.clear(); }

      static QString key(const AlbumQuery &query);

    private:
      Cache();

      DiskCache::Store store;
    };
  }
}

#endif // COVERART_ONLINE_CACHE_H
