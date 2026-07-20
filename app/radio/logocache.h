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
  class LogoCache : public QObject {
    Q_OBJECT

  public:
    static LogoCache &instance();

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
