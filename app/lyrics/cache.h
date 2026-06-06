#ifndef LYRICS_CACHE_H
#define LYRICS_CACHE_H

#include "lyrics/trackquery.h"

#include <QHash>
#include <QString>

namespace Lyrics {
  // Session-only in-memory cache of online lyrics lookups, keyed by
  // artist + title. Caches both hits (lyrics + the provider that found them)
  // and genuine misses, so reopening the track info dialog doesn't re-walk
  // the provider chain.
  class Cache {
  public:
    struct Entry {
      bool found = false;
      QString provider;
      QString lyrics;
    };

    Cache() = default;
    static Cache &instance();

    // Returns true on a cache hit; out.found distinguishes lyrics from a
    // known miss.
    bool lookup(const TrackQuery &query, Entry &out) const;
    void storeFound(const TrackQuery &query, const QString &provider, const QString &lyrics);
    void storeNotFound(const TrackQuery &query);

    static QString key(const TrackQuery &query);

  private:
    QHash<QString, Entry> entries;
  };
}

#endif // LYRICS_CACHE_H
