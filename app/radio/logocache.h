#ifndef RADIO_LOGOCACHE_H
#define RADIO_LOGOCACHE_H

#include "diskcache.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QString>

namespace Radio {
  // Lazily fetches station logos and keeps them in the shared disk cache. The
  // only network the radio tree does: one request per station, on first paint,
  // never repeated within a run. Stations without a logo_url, and stations whose
  // fetch failed, fall back to the delegate's colour badge forever.
  class LogoCache : public QObject {
    Q_OBJECT

  public:
    static LogoCache &instance();

    // Returns the logo if already available, otherwise a null pixmap and
    // schedules a fetch that ends in logoAvailable.
    QPixmap get(const QString &station_id, const QString &logo_url);

  signals:
    void logoAvailable(const QString &station_id);

  private:
    explicit LogoCache(QObject *parent = nullptr);

    QPixmap loadFromDisk(const QString &station_id);
    void fetch(const QString &station_id, const QString &logo_url);

    DiskCache::Store store;
    QNetworkAccessManager nam;
    QHash<QString, QPixmap> memory;
    QSet<QString> in_flight;
    QSet<QString> failed;
  };
}

#endif // RADIO_LOGOCACHE_H
