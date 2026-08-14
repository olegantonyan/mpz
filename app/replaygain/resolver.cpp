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
    settings_ = s;
  }

  void Resolver::invalidate() {
    cache.clear();
  }

  void Resolver::invalidate(const QString &path, quint64 begin_ms) {
    cache.remove(Store::Key(path, begin_ms));
  }

  Store::Key Resolver::keyFor(const Track &track) {
    return Store::Key(track.path(), track.begin());
  }

  Resolved Resolver::resolve(const Track &track) {
    if (track.isStream() || track.isMpd() || track.path().isEmpty()) {
      return Resolved();
    }

    const Store::Key key = keyFor(track);
    if (const Resolved *cached = cache.object(key)) {
      return *cached;
    }

    // the store is written in both modes, and a fresh scan lands there first
    Resolved r;
    if (store_) {
      r.gain = store_->get(track.path(), track.begin());
      if (r.gain.isValid()) {
        r.source = Source::Sidecar;
      }
    }
    if (!r.gain.isValid()) {
      r.gain = track.replayGain();
      if (r.gain.isValid()) {
        r.source = Source::Cue;
      }
    }
    if (!r.gain.isValid() && !track.isCue()) {
      // A cue container's tags describe the whole file, not one slice.
      r.gain = readTags(track.path());
      if (r.gain.isValid()) {
        r.source = Source::Tags;
      }
    }

    cache.insert(key, new Resolved(r));
    return r;
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
