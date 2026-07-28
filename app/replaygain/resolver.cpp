#include "replaygain/resolver.h"

#include "replaygain/store.h"
#include "replaygain/tags.h"

namespace ReplayGain {
  Resolver::Resolver(Store *store) : store_(store) {
  }

  void Resolver::setStore(Store *s) {
    store_ = s;
    cache.clear();
  }

  void Resolver::setSettings(const Settings &s) {
    if (s.storage != settings_.storage) {
      cache.clear();
    }
    settings_ = s;
  }

  void Resolver::invalidate() {
    cache.clear();
  }

  void Resolver::invalidate(const Track &track) {
    cache.remove(keyFor(track));
  }

  QString Resolver::keyFor(const Track &track) {
    return Store::key(track.path(), track.begin());
  }

  Gain Resolver::gainFor(const Track &track) {
    if (track.isStream() || track.isMpd() || track.path().isEmpty()) {
      return Gain();
    }

    const QString key = keyFor(track);
    const auto cached = cache.constFind(key);
    if (cached != cache.constEnd()) {
      return *cached;
    }

    Gain g;
    if (settings_.storage == StorageMode::Sidecar && store_) {
      g = store_->get(track.path(), track.begin());
    }
    if (!g.isValid()) {
      g = track.replayGain();
    }
    if (!g.isValid() && !track.isCue()) {
      // A cue container's tags describe the whole file, not one slice.
      g = readTags(track.path());
    }

    cache.insert(key, g);
    return g;
  }

  double Resolver::gainDbFor(const Track &track) {
    if (settings_.mode == Mode::Off) {
      return 0.0;
    }
    if (track.isStream()) {
      return 0.0;
    }
    return effectiveGainDb(gainFor(track), settings_);
  }
}
