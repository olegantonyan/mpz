#include "playback/gapless/pcmcache.h"

#include <cstring>

namespace Playback::Gapless {
  PcmCache::PcmCache(qint64 budget_bytes, qint64 chunk_bytes)
      : budget(budget_bytes), chunk_bytes(chunk_bytes) {
  }

  PcmCache::Entry *PcmCache::entryFor(const QUrl &url) {
    auto it = entries.find(url);
    return it == entries.end() ? nullptr : &it.value();
  }

  const PcmCache::Entry *PcmCache::entryFor(const QUrl &url) const {
    auto it = entries.constFind(url);
    return it == entries.constEnd() ? nullptr : &it.value();
  }

  void PcmCache::openEntry(const QUrl &url, int bytes_per_frame) {
    if (prebuffer_url == url) {
      prebuffer_url = QUrl();
    }
    Entry e;
    e.bytes_per_frame = bytes_per_frame > 0 ? bytes_per_frame : 1;
    e.chunk_capacity = qMax<qint64>(e.bytes_per_frame,
                                    (chunk_bytes / e.bytes_per_frame) * e.bytes_per_frame);
    entries.insert(url, e);
    current_url = url;
    recomputeAllowances();
  }

  void PcmCache::setPrebufferEntry(const QUrl &url, int bytes_per_frame, qint64 max_bytes) {
    Entry e;
    e.bytes_per_frame = bytes_per_frame > 0 ? bytes_per_frame : 1;
    e.chunk_capacity = qMax<qint64>(e.bytes_per_frame,
                                    (chunk_bytes / e.bytes_per_frame) * e.bytes_per_frame);
    e.prebuffer = true;
    e.max_bytes = qMax<qint64>(0, max_bytes);
    entries.insert(url, e);
    prebuffer_url = url;
    recomputeAllowances();
  }

  void PcmCache::promote(const QUrl &url) {
    if (!entries.contains(url)) {
      return;
    }
    const auto keys = entries.keys();
    for (const QUrl &k : keys) {
      if (k != url) {
        entries.remove(k);
      }
    }
    Entry &e = entries[url];
    e.prebuffer = false;
    e.max_bytes = 0;
    current_url = url;
    prebuffer_url = QUrl();
    recomputeAllowances();
  }

  void PcmCache::dropEntry(const QUrl &url) {
    entries.remove(url);
    if (current_url == url) {
      current_url = QUrl();
    }
    if (prebuffer_url == url) {
      prebuffer_url = QUrl();
    }
    recomputeAllowances();
  }

  void PcmCache::clear() {
    entries.clear();
    current_url = QUrl();
    prebuffer_url = QUrl();
  }

  void PcmCache::recomputeAllowances() {
    qint64 pre_max = 0;
    if (Entry *pre = entryFor(prebuffer_url)) {
      pre->allowance = pre->max_bytes;
      pre_max = pre->max_bytes;
    }
    if (Entry *cur = entryFor(current_url)) {
      cur->allowance = qMax<qint64>(0, budget - pre_max);
    }
  }

  bool PcmCache::frontEvictable(const Entry &e) const {
    if (e.chunks.empty()) {
      return false;
    }
    const qint64 chunk_end_frame = (e.first_byte + e.chunks.front().size()) / e.bytes_per_frame;
    return chunk_end_frame <= e.protect_frame;
  }

  void PcmCache::evict(Entry &e) {
    // The protect floor beats the budget: frames >= protect_frame are never dropped,
    // so retained may exceed the allowance while the floor pins the front.
    while (e.retained() > e.allowance && frontEvictable(e)) {
      e.first_byte += e.chunks.front().size();
      e.chunks.pop_front();
    }
  }

  void PcmCache::append(const QUrl &url, const char *data, qint64 bytes) {
    Entry *e = entryFor(url);
    if (!e || bytes <= 0) {
      return;
    }
    qint64 remaining = bytes;
    const char *p = data;
    while (remaining > 0) {
      if (e->chunks.empty() || e->chunks.back().size() >= e->chunk_capacity) {
        e->chunks.emplace_back();
        e->chunks.back().reserve(static_cast<int>(e->chunk_capacity));
      }
      QByteArray &c = e->chunks.back();
      const qint64 space = e->chunk_capacity - c.size();
      const qint64 n = qMin(space, remaining);
      c.append(p, static_cast<int>(n));
      p += n;
      remaining -= n;
    }
    e->frontier_byte += bytes;
    evict(*e);
  }

  qint64 PcmCache::frontierFrame(const QUrl &url) const {
    const Entry *e = entryFor(url);
    return e ? e->frontier_byte / e->bytes_per_frame : 0;
  }

  qint64 PcmCache::firstFrame(const QUrl &url) const {
    const Entry *e = entryFor(url);
    return e ? e->first_byte / e->bytes_per_frame : 0;
  }

  bool PcmCache::contains(const QUrl &url, qint64 frame) const {
    const Entry *e = entryFor(url);
    if (!e) {
      return false;
    }
    return frame >= e->first_byte / e->bytes_per_frame && frame < e->frontier_byte / e->bytes_per_frame;
  }

  qint64 PcmCache::read(const QUrl &url, qint64 from_frame, char *dst, qint64 max_frames) const {
    const Entry *e = entryFor(url);
    if (!e || max_frames <= 0 || !contains(url, from_frame)) {
      return 0;
    }
    const int bpf = e->bytes_per_frame;
    const qint64 avail = e->frontier_byte / bpf - from_frame;
    const qint64 nframes = qMin(max_frames, avail);
    if (nframes <= 0) {
      return 0;
    }
    const qint64 to_copy = nframes * bpf;
    qint64 skip = from_frame * bpf - e->first_byte;
    qint64 copied = 0;
    for (const QByteArray &c : e->chunks) {
      if (skip >= c.size()) {
        skip -= c.size();
        continue;
      }
      const qint64 off = skip;
      skip = 0;
      const qint64 n = qMin<qint64>(c.size() - off, to_copy - copied);
      std::memcpy(dst + copied, c.constData() + off, static_cast<size_t>(n));
      copied += n;
      if (copied >= to_copy) {
        break;
      }
    }
    return copied / bpf;
  }

  void PcmCache::protect(const QUrl &url, qint64 floor_frame) {
    if (Entry *e = entryFor(url)) {
      e->protect_frame = floor_frame;
    }
  }

  bool PcmCache::wantsMoreData(const QUrl &url) const {
    const Entry *e = entryFor(url);
    if (!e) {
      return false;
    }
    return e->retained() < e->allowance || frontEvictable(*e);
  }

  qint64 PcmCache::totalBytes() const {
    qint64 total = 0;
    for (const Entry &e : entries) {
      total += e.retained();
    }
    return total;
  }
}
