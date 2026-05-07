#include "lyrics/lrclibclient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace Lyrics {
  LrcLibClient::LrcLibClient(QObject *parent) : QObject(parent) {
  }

  void LrcLibClient::fetch(const QString &artist, const QString &title, const QString &album, int duration_seconds) {
    QUrl url("https://lrclib.net/api/get");
    QUrlQuery q;
    q.addQueryItem("artist_name", artist);
    q.addQueryItem("track_name", title);
    if (!album.isEmpty()) {
      q.addQueryItem("album_name", album);
    }
    if (duration_seconds > 0) {
      q.addQueryItem("duration", QString::number(duration_seconds));
    }
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QString("mpz/%1 (https://github.com/olegantonyan/mpz)").arg(VERSION));

    auto *reply = nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, artist, title]() {
      reply->deleteLater();
      auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      if (reply->error() == QNetworkReply::ContentNotFoundError || status == 404) {
        searchFallback(artist, title);
        return;
      }
      if (reply->error() != QNetworkReply::NoError) {
        emit failed(reply->errorString());
        return;
      }
      auto body = reply->readAll();
      QJsonParseError perr;
      auto doc = QJsonDocument::fromJson(body, &perr);
      if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit failed(QString("invalid response from lrclib"));
        return;
      }
      auto obj = doc.object();
      if (obj.value("instrumental").toBool()) {
        emit notFound();
        return;
      }
      auto synced = obj.value("syncedLyrics").toString();
      auto plain = obj.value("plainLyrics").toString();
      QString result = !synced.isEmpty() ? synced : plain;
      if (result.isEmpty()) {
        searchFallback(artist, title);
        return;
      }
      emit found(result);
    });
  }

  void LrcLibClient::searchFallback(const QString &artist, const QString &title) {
    QUrl url("https://lrclib.net/api/search");
    QUrlQuery q;
    if (!artist.isEmpty()) {
      q.addQueryItem("artist_name", artist);
    }
    q.addQueryItem("track_name", title);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QString("mpz/%1 (https://github.com/olegantonyan/mpz)").arg(VERSION));

    auto *reply = nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, artist]() {
      reply->deleteLater();
      if (reply->error() != QNetworkReply::NoError) {
        emit notFound();
        return;
      }
      auto body = reply->readAll();
      QJsonParseError perr;
      auto doc = QJsonDocument::fromJson(body, &perr);
      if (perr.error != QJsonParseError::NoError || !doc.isArray()) {
        emit notFound();
        return;
      }
      const QString needle = artist.trimmed().toLower();
      for (const auto &v : doc.array()) {
        if (!v.isObject()) {
          continue;
        }
        auto obj = v.toObject();
        if (obj.value("instrumental").toBool()) {
          continue;
        }
        const QString cand = obj.value("artistName").toString().trimmed().toLower();
        if (!needle.isEmpty() && !cand.contains(needle) && !needle.contains(cand)) {
          continue;
        }
        const QString synced = obj.value("syncedLyrics").toString();
        const QString plain = obj.value("plainLyrics").toString();
        const QString result = !synced.isEmpty() ? synced : plain;
        if (!result.isEmpty()) {
          emit found(result);
          return;
        }
      }
      emit notFound();
    });
  }
}
