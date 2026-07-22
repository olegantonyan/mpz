#ifndef PCMCACHE_H
#define PCMCACHE_H

#include <QByteArray>
#include <QHash>
#include <QUrl>

#include <deque>
#include <limits>

namespace Playback::Gapless {
  class PcmCache {
  public:
    explicit PcmCache(qint64 budget_bytes, qint64 chunk_bytes = 256 * 1024);

    void openEntry(const QUrl &url, int bytes_per_frame);
    void setPrebufferEntry(const QUrl &url, int bytes_per_frame, qint64 max_bytes);
    void promote(const QUrl &url);
    void dropEntry(const QUrl &url);
    void clear();

    void append(const QUrl &url, const char *data, qint64 bytes);

    qint64 frontierFrame(const QUrl &url) const;
    qint64 firstFrame(const QUrl &url) const;
    bool contains(const QUrl &url, qint64 frame) const;
    qint64 read(const QUrl &url, qint64 from_frame, char *dst, qint64 max_frames) const;

    void protect(const QUrl &url, qint64 floor_frame);
    bool wantsMoreData(const QUrl &url) const;
    qint64 totalBytes() const;

  private:
    static constexpr qint64 kNoFloor = std::numeric_limits<qint64>::max();

    struct Entry {
      int bytes_per_frame = 0;
      qint64 chunk_capacity = 0;
      qint64 allowance = 0;
      qint64 max_bytes = 0;
      bool prebuffer = false;
      std::deque<QByteArray> chunks;
      qint64 first_byte = 0;
      qint64 frontier_byte = 0;
      qint64 protect_frame = kNoFloor;

      qint64 retained() const { return frontier_byte - first_byte; }
    };

    Entry *entryFor(const QUrl &url);
    const Entry *entryFor(const QUrl &url) const;
    void recomputeAllowances();
    void evict(Entry &e);
    bool frontEvictable(const Entry &e) const;

    qint64 budget;
    qint64 chunk_bytes;
    QHash<QUrl, Entry> entries;
    QUrl current_url;
    QUrl prebuffer_url;
  };
}

#endif // PCMCACHE_H
