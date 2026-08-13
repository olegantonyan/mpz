#ifndef REPLAYGAIN_RESOLVER_H
#define REPLAYGAIN_RESOLVER_H

#include "replaygain/gain.h"
#include "track.h"
#include "replaygain/store.h"

#include <QHash>
#include <QString>

namespace ReplayGain {
  class Resolver {
  public:
    explicit Resolver(Store *store = nullptr);

    void setStore(Store *s);
    void setSettings(const Settings &s);
    Settings settings() const { return settings_; }

    Gain gainFor(const Track &track);
    double gainDbFor(const Track &track);

    void invalidate();
    void invalidate(const Track &track);

  private:
    static QString keyFor(const Track &track);

    Store *store_ = nullptr;
    Settings settings_;
    QHash<QString, Gain> cache;
  };
}

#endif // REPLAYGAIN_RESOLVER_H
