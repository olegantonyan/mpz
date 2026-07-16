#include "lyrics/providerchain.h"
#include "lyrics/cache.h"
#include "lyrics/lrclibclient.h"
#include "lyrics/lyricsovhclient.h"
#include "lyrics/neteaseclient.h"
#include "lyrics/qqmusicclient.h"

#include <QDebug>

namespace Lyrics {
  ProviderChain::ProviderChain(QObject *parent) : QObject(parent) {
    watchdog.setSingleShot(true);
    // Backstop for a provider that never emits; per-request transfer timeouts
    // normally fire first.
    watchdog.setInterval(20000);
  }

  QStringList ProviderChain::knownProviders() {
    return {"lrclib", "netease", "qq", "lyrics.ovh"};
  }

  QString ProviderChain::displayName(const QString &name) {
    if (name == "lrclib") {
      return QStringLiteral("LRCLIB");
    }
    if (name == "netease") {
      return QStringLiteral("NetEase");
    }
    if (name == "qq") {
      return QStringLiteral("QQ Music");
    }
    if (name == "lyrics.ovh") {
      return QStringLiteral("Lyrics.ovh");
    }
    return name;
  }

  QStringList ProviderChain::filterKnown(const QStringList &names) {
    const auto known = knownProviders();
    QStringList result;
    for (const auto &name : names) {
      if (known.contains(name)) {
        result << name;
      }
    }
    return result;
  }

  void ProviderChain::fetch(const QStringList &enabled_providers, const TrackQuery &q) {
    query = q;
    enabled = filterKnown(enabled_providers);
    pending = enabled;

    Cache::Entry cached;
    if (Cache::instance().lookup(query, cached)) {
      emit found(cached.provider, cached.lyrics);
      return;
    }
    if (pending.isEmpty()) {
      // No provider consulted — don't record a negative entry.
      emit notFound();
      return;
    }
    if (Cache::instance().isKnownMiss(query, enabled)) {
      emit notFound();
      return;
    }
    had_failure = false;
    advance();
  }

  void ProviderChain::advance() {
    watchdog.stop();
    watchdog.disconnect();
    if (pending.isEmpty()) {
      if (!had_failure) {
        // Every provider genuinely returned not-found; a failed/timed-out
        // provider must not poison the cache with a 30-day sentinel.
        Cache::instance().storeNotFound(query, enabled);
      }
      emit notFound();
      return;
    }
    const QString name = pending.takeFirst();
    auto *provider = makeProvider(name);
    if (!provider) {
      advance();
      return;
    }
    connect(provider, &Provider::found, this, [this, name, provider](const QString &lyrics) {
      watchdog.stop();
      provider->disconnect(this);
      provider->deleteLater();
      Cache::instance().storeFound(query, name, lyrics);
      emit found(name, lyrics);
    });
    connect(provider, &Provider::notFound, this, [this, provider]() {
      provider->disconnect(this);
      provider->deleteLater();
      advance();
    });
    connect(provider, &Provider::failed, this, [this, name, provider](const QString &message) {
      qWarning() << "lyrics provider" << name << "failed:" << message;
      had_failure = true;
      provider->disconnect(this);
      provider->deleteLater();
      advance();
    });
    connect(&watchdog, &QTimer::timeout, this, [this, name, provider]() {
      qWarning() << "lyrics provider" << name << "timed out";
      had_failure = true;
      provider->disconnect(this);
      provider->deleteLater();
      advance();
    });
    watchdog.start();
    provider->fetch(query);
  }

  Provider *ProviderChain::makeProvider(const QString &name) {
    if (name == "lrclib") {
      return new LrcLibClient(this);
    }
    if (name == "netease") {
      return new NetEaseClient(this);
    }
    if (name == "qq") {
      return new QQMusicClient(this);
    }
    if (name == "lyrics.ovh") {
      return new LyricsOvhClient(this);
    }
    return nullptr;
  }
}
