#ifndef LYRICS_PROVIDERCHAIN_H
#define LYRICS_PROVIDERCHAIN_H

#include "lyrics/provider.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace Lyrics {
  // Walks enabled online providers in the user-configured order, advancing on
  // notFound/failed. Emits found(provider_name, lyrics) or notFound() once.
  // Results are persisted (see Cache); a cache hit emits synchronously from
  // fetch(), so connect before fetching.
  class ProviderChain : public QObject {
    Q_OBJECT
  public:
    explicit ProviderChain(QObject *parent = nullptr);

    static QStringList knownProviders();
    static QString displayName(const QString &name);
    // Drops names this chain can't serve, preserving order. Configs written
    // before built-in sources became always-on still list "embedded"/"sidecar".
    static QStringList filterKnown(const QStringList &names);

    void fetch(const QStringList &enabled_providers, const TrackQuery &query);

  signals:
    void found(const QString &provider_name, const QString &lyrics);
    void notFound();

  private:
    void advance();
    Provider *makeProvider(const QString &name);

    QStringList pending;
    QStringList enabled;
    TrackQuery query;
    QTimer watchdog;
    bool had_failure = false;
  };
}

#endif // LYRICS_PROVIDERCHAIN_H
