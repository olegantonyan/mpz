#ifndef DISKCACHE_H
#define DISKCACHE_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace DiskCache {
  // A hashed file cache under QStandardPaths::CacheLocation, shared by the
  // online lyrics and album cover caches so the two can't drift apart.
  //
  // Unlike the scratch dirs in CoverArt::Embedded/Mpd this is never wiped on
  // exit — it is the point of the thing — so it is capped instead, and cleared
  // only when the user asks.
  class Store {
  public:
    explicit Store(const QString &subdir);

    QString dir() const { return cache_dir; }

    // Path of the cached file for key with any of exts, or empty.
    QString find(const QString &key, const QStringList &exts) const;
    // Writes atomically; returns the path, or empty on failure.
    QString write(const QString &key, const QString &ext, const QByteArray &data);

    // Negative results are tagged with the provider set that produced them, so
    // enabling another provider retries instead of inheriting the old miss.
    bool isKnownMiss(const QString &key, const QStringList &tags) const;
    void storeMiss(const QString &key, const QStringList &tags);

    // Drops everything, including miss sentinels. Returns the file count.
    int clear();
    // Keeps the newest max_entries files matching exts.
    void trim(const QStringList &exts, int max_entries) const;

    // Stable key for a two-part identity (artist + album, artist + title).
    // Empty when either part is empty, which disables caching for that item.
    static QString hash(const QString &first, const QString &second);

  private:
    QString missPath(const QString &key, const QStringList &tags) const;

    QString cache_dir;
  };
}

#endif // DISKCACHE_H
