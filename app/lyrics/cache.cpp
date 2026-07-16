#include "lyrics/cache.h"
#include "lyrics/textmatch.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

namespace Lyrics {
  namespace {
    const QStringList EXTENSIONS = {"json"};
    const int MAX_CACHED_LYRICS = 5000;
  }

  Cache &Cache::instance() {
    static Cache self;
    return self;
  }

  Cache::Cache() : store("downloaded_lyrics") {
  }

  QString Cache::key(const TrackQuery &query) {
    // Album and duration are deliberately excluded: the same song on another
    // album or compilation should hit the cache.
    return DiskCache::Store::hash(TextMatch::normalizeArtist(query.artist),
                                  TextMatch::normalizeTitle(query.title));
  }

  bool Cache::lookup(const TrackQuery &query, Entry &out) const {
    const QString path = store.find(key(query), EXTENSIONS);
    if (path.isEmpty()) {
      return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
      return false;
    }
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
      return false;
    }
    const auto obj = doc.object();
    const QString lyrics = obj.value("lyrics").toString();
    if (lyrics.isEmpty()) {
      return false;
    }
    out.provider = obj.value("provider").toString();
    out.lyrics = lyrics;
    return true;
  }

  bool Cache::isKnownMiss(const TrackQuery &query, const QStringList &providers) const {
    return store.isKnownMiss(key(query), providers);
  }

  void Cache::storeFound(const TrackQuery &query, const QString &provider, const QString &lyrics) {
    if (lyrics.isEmpty()) {
      return;
    }
    QJsonObject obj;
    obj.insert("provider", provider);
    obj.insert("lyrics", lyrics);
    const QString path = store.write(key(query), "json", QJsonDocument(obj).toJson(QJsonDocument::Compact));
    if (!path.isEmpty()) {
      store.trim(EXTENSIONS, MAX_CACHED_LYRICS);
    }
  }

  void Cache::storeNotFound(const TrackQuery &query, const QStringList &providers) {
    store.storeMiss(key(query), providers);
  }
}
