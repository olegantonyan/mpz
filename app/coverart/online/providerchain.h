#ifndef COVERART_ONLINE_PROVIDERCHAIN_H
#define COVERART_ONLINE_PROVIDERCHAIN_H

#include "coverart/online/provider.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace CoverArt {
  namespace Online {
    // Walks enabled online providers in the user-configured order, advancing on
    // notFound/failed. Emits found(provider_name, path) or notFound() once.
    // Results are persisted (see Cache); a cache hit emits synchronously from
    // fetch(), so connect before fetching.
    class ProviderChain : public QObject {
      Q_OBJECT
    public:
      explicit ProviderChain(QObject *parent = nullptr);

      static QStringList knownProviders();
      static QString displayName(const QString &name);
      // Drops names this chain can't serve, preserving order.
      static QStringList filterKnown(const QStringList &names);

      void fetch(const QStringList &enabled_providers, const AlbumQuery &query);

    signals:
      void found(const QString &provider_name, const QString &path);
      void notFound();

    private:
      void advance();
      Provider *makeProvider(const QString &name);

      QStringList pending;
      QStringList enabled;
      AlbumQuery query;
      QTimer watchdog;
      bool had_failure = false;
    };
  }
}

#endif // COVERART_ONLINE_PROVIDERCHAIN_H
