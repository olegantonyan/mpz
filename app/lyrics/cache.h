#ifndef LYRICS_CACHE_H
#define LYRICS_CACHE_H

#include "lyrics/trackquery.h"
#include "diskcache.h"

#include <QString>
#include <QStringList>

namespace Lyrics {
  // Persistent on-disk cache of online lyrics lookups, keyed by artist + title.
  // Same shape as CoverArt::Online::Cache, sharing DiskCache::Store.
  //
  // Only online results land here: built-in sources (embedded tags, sidecar
  // files) are local reads and are always consulted first anyway.
  class Cache {
  public:
    struct Entry {
      QString provider;
      QString lyrics;
    };

    static Cache &instance();

    QString dir() const { return store.dir(); }

    // True on a hit, filling out.
    bool lookup(const TrackQuery &query, Entry &out) const;
    // True when a previous search with this exact provider set came up empty
    // and the sentinel hasn't expired yet.
    bool isKnownMiss(const TrackQuery &query, const QStringList &providers) const;

    void storeFound(const TrackQuery &query, const QString &provider, const QString &lyrics);
    void storeNotFound(const TrackQuery &query, const QStringList &providers);

    // Removes every cached lyric and miss sentinel. Returns the file count.
    int clear() { return store.clear(); }

    static QString key(const TrackQuery &query);

  private:
    Cache();

    DiskCache::Store store;
  };
}

#endif // LYRICS_CACHE_H
