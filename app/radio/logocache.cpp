#include "radio/logocache.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImage>
#include <QNetworkReply>
#include <QUrl>

namespace Radio {
  namespace {
    const qint64 MAX_LOGO_BYTES = 2 * 1024 * 1024;
    const int MAX_CACHED_LOGOS = 500;
    const QStringList LOGO_EXTENSIONS = {"png", "jpg", "jpeg", "webp"};

    QString cacheKey(const QString &station_id) {
      return QCryptographicHash::hash(station_id.toUtf8(), QCryptographicHash::Sha1).toHex();
    }
  }

  LogoCache::LogoCache(QObject *parent) : QObject(parent), store(QStringLiteral("radio")) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    nam.setTransferTimeout(10000);
#endif
    // Logo URLs commonly redirect to a CDN; Qt5 does not follow by default.
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
  }

  LogoCache &LogoCache::instance() {
    static LogoCache cache;
    return cache;
  }

  QPixmap LogoCache::loadFromDisk(const QString &station_id) {
    const auto path = store.find(cacheKey(station_id), LOGO_EXTENSIONS);
    if (path.isEmpty()) {
      return QPixmap();
    }
    QPixmap pm;
    if (!pm.load(path)) {
      return QPixmap();
    }
    return pm;
  }

  QPixmap LogoCache::get(const QString &station_id, const QString &logo_url) {
    if (station_id.isEmpty()) {
      return QPixmap();
    }

    const auto cached = memory.constFind(station_id);
    if (cached != memory.constEnd()) {
      return cached.value();
    }

    auto from_disk = loadFromDisk(station_id);
    if (!from_disk.isNull()) {
      memory.insert(station_id, from_disk);
      return from_disk;
    }

    if (logo_url.isEmpty() || failed.contains(station_id) || in_flight.contains(station_id)) {
      return QPixmap();
    }
    if (store.isKnownMiss(cacheKey(station_id), {QStringLiteral("logo")})) {
      failed.insert(station_id);
      return QPixmap();
    }

    fetch(station_id, logo_url);
    return QPixmap();
  }

  void LogoCache::fetch(const QString &station_id, const QString &logo_url) {
    const QUrl url(logo_url);
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https")) {
      failed.insert(station_id);
      return;
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QString("mpz/%1").arg(qApp->applicationVersion()));

    in_flight.insert(station_id);
    auto *reply = nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, station_id]() {
      reply->deleteLater();
      in_flight.remove(station_id);

      const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      const auto data = reply->readAll();
      if (reply->error() != QNetworkReply::NoError || (status != 0 && status != 200)
          || data.isEmpty() || data.size() > MAX_LOGO_BYTES) {
        failed.insert(station_id);
        store.storeMiss(cacheKey(station_id), {QStringLiteral("logo")});
        return;
      }

      QImage image;
      if (!image.loadFromData(data)) {
        failed.insert(station_id);
        store.storeMiss(cacheKey(station_id), {QStringLiteral("logo")});
        return;
      }

      QString ext = QFileInfo(reply->url().path()).suffix().toLower();
      if (!LOGO_EXTENSIONS.contains(ext)) {
        ext = QStringLiteral("png");
      }
      store.write(cacheKey(station_id), ext, data);
      store.trim(LOGO_EXTENSIONS, MAX_CACHED_LOGOS);

      memory.insert(station_id, QPixmap::fromImage(image));
      emit logoAvailable(station_id);
    });
  }
}
