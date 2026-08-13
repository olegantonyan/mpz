#ifndef REPLAYGAIN_RESOLVER_H
#define REPLAYGAIN_RESOLVER_H

#include "replaygain/gain.h"
#include "track.h"
#include "replaygain/store.h"

#include <QHash>
#include <QString>

namespace ReplayGain {
  enum class Source { None, Sidecar, Tags, Cue };

  struct Resolved {
    Gain gain;
    Source source = Source::None;
  };

  class Resolver {
  public:
    explicit Resolver(Store *store = nullptr);

    void setStore(Store *s);
    void setSettings(const Settings &s);
    Settings settings() const { return settings_; }

    Resolved resolve(const Track &track);
    Gain gainFor(const Track &track) { return resolve(track).gain; }
    double gainDbFor(const Track &track);

    void invalidate();
    void invalidate(const Track &track);
    void invalidate(const QString &path, quint64 begin_ms);

  private:
    static Store::Key keyFor(const Track &track);

    Store *store_ = nullptr;
    Settings settings_;
    QHash<Store::Key, Resolved> cache;
  };
}

#endif // REPLAYGAIN_RESOLVER_H
