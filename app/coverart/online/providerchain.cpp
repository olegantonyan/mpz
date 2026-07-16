#include "coverart/online/providerchain.h"
#include "coverart/online/cache.h"
#include "coverart/online/coverartarchiveclient.h"
#include "coverart/online/deezerclient.h"
#include "coverart/online/itunesclient.h"

#include <QDebug>

namespace CoverArt {
  namespace Online {
    ProviderChain::ProviderChain(QObject *parent) : QObject(parent) {
      watchdog.setSingleShot(true);
      // Backstop for a provider that never emits; per-request transfer timeouts
      // normally fire first.
      watchdog.setInterval(20000);
    }

    QStringList ProviderChain::knownProviders() {
      // Cover Art Archive is last: two round-trips and a 1 req/sec limit make it
      // the slowest, so it only runs when the fast ones come up empty.
      return {"deezer", "itunes", "coverartarchive"};
    }

    QString ProviderChain::displayName(const QString &name) {
      if (name == "deezer") {
        return QStringLiteral("Deezer");
      }
      if (name == "itunes") {
        return QStringLiteral("Apple Music");
      }
      if (name == "coverartarchive") {
        return QStringLiteral("Cover Art Archive");
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

    void ProviderChain::fetch(const QStringList &enabled_providers, const AlbumQuery &q) {
      query = q;
      enabled = filterKnown(enabled_providers);
      pending = enabled;

      const QString cached = Cache::instance().lookup(query);
      if (!cached.isEmpty()) {
        emit found(QString(), cached);
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
      connect(provider, &Provider::found, this, [this, name, provider](const QByteArray &image, const QString &format) {
        watchdog.stop();
        provider->disconnect(this);
        provider->deleteLater();
        const QString path = Cache::instance().storeFound(query, image, format);
        if (path.isEmpty()) {
          qWarning() << "cover provider" << name << "found art but it could not be cached";
          emit notFound();
          return;
        }
        emit found(name, path);
      });
      connect(provider, &Provider::notFound, this, [this, provider]() {
        provider->disconnect(this);
        provider->deleteLater();
        advance();
      });
      connect(provider, &Provider::failed, this, [this, name, provider](const QString &message) {
        qWarning() << "cover provider" << name << "failed:" << message;
        had_failure = true;
        provider->disconnect(this);
        provider->deleteLater();
        advance();
      });
      connect(&watchdog, &QTimer::timeout, this, [this, name, provider]() {
        qWarning() << "cover provider" << name << "timed out";
        had_failure = true;
        provider->disconnect(this);
        provider->deleteLater();
        advance();
      });
      watchdog.start();
      provider->fetch(query);
    }

    Provider *ProviderChain::makeProvider(const QString &name) {
      if (name == "deezer") {
        return new DeezerClient(this);
      }
      if (name == "itunes") {
        return new ItunesClient(this);
      }
      if (name == "coverartarchive") {
        return new CoverArtArchiveClient(this);
      }
      return nullptr;
    }
  }
}
