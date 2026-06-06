#include "lyrics/cache.h"

namespace Lyrics {
  Cache &Cache::instance() {
    static Cache self;
    return self;
  }

  bool Cache::lookup(const TrackQuery &query, Entry &out) const {
    const auto k = key(query);
    if (k.isEmpty()) {
      return false;
    }
    auto it = entries.constFind(k);
    if (it == entries.constEnd()) {
      return false;
    }
    out = it.value();
    return true;
  }

  void Cache::storeFound(const TrackQuery &query, const QString &provider, const QString &lyrics) {
    const auto k = key(query);
    if (k.isEmpty()) {
      return;
    }
    entries.insert(k, Entry{true, provider, lyrics});
  }

  void Cache::storeNotFound(const TrackQuery &query) {
    const auto k = key(query);
    if (k.isEmpty()) {
      return;
    }
    entries.insert(k, Entry{});
  }

  QString Cache::key(const TrackQuery &query) {
    const QString artist = query.artist.trimmed().toCaseFolded();
    const QString title = query.title.trimmed().toCaseFolded();
    if (artist.isEmpty() || title.isEmpty()) {
      return QString();
    }
    // Album and duration are deliberately excluded: the same song on another
    // album/compilation should hit the cache.
    return artist + QChar(0x1F) + title;
  }
}
