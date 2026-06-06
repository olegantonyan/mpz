#include "lyrics/qqmusicclient.h"
#include "lyrics/lrcparser.h"
#include "lyrics/textmatch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>

namespace Lyrics {
  QQMusicClient::QQMusicClient(QObject *parent) : Provider(parent) {
  }

  void QQMusicClient::fetch(const TrackQuery &q) {
    query = q;

    const QString artist = TextMatch::normalizeArtist(q.artist);
    const QString title = TextMatch::normalizeTitle(q.title);
    const QString key = artist.isEmpty() ? title : artist + " " + title;

    QUrl url("https://c.y.qq.com/splcloud/fcgi-bin/smartbox_new.fcg");
    QUrlQuery uq;
    uq.addQueryItem("key", key);
    uq.addQueryItem("format", "json");
    uq.addQueryItem("inCharset", "utf-8");
    uq.addQueryItem("outCharset", "utf-8");
    url.setQuery(uq);

    auto req = makeRequest(url, true);
    req.setRawHeader("Referer", "https://y.qq.com/portal/player.html");

    auto *reply = nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSearchReply(reply); });
  }

  void QQMusicClient::handleSearchReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit failed(reply->errorString());
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(stripJsonp(reply->readAll()), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
      emit notFound();
      return;
    }
    QVector<SongCandidate> candidates;
    QVector<QString> mids; // candidate index -> song mid
    const auto items = doc.object().value("data").toObject()
                          .value("song").toObject()
                          .value("itemlist").toArray();
    for (const auto &v : items) {
      if (!v.isObject()) {
        continue;
      }
      const auto item = v.toObject();
      const QString mid = item.value("mid").toString();
      if (mid.isEmpty()) {
        continue;
      }
      SongCandidate c;
      c.title = item.value("name").toString();
      c.artist = item.value("singer").toString();
      candidates.append(c);
      mids.append(mid);
    }
    const int best = TextMatch::bestCandidate(candidates, query.artist, query.title, query.duration_seconds);
    if (best < 0) {
      emit notFound();
      return;
    }
    fetchLyric(mids.at(best));
  }

  void QQMusicClient::fetchLyric(const QString &song_mid) {
    QUrl url("https://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg");
    QUrlQuery uq;
    uq.addQueryItem("songmid", song_mid);
    uq.addQueryItem("format", "json");
    uq.addQueryItem("g_tk", "5381");
    uq.addQueryItem("inCharset", "utf8");
    uq.addQueryItem("outCharset", "utf-8");
    url.setQuery(uq);

    auto req = makeRequest(url, true);
    req.setRawHeader("Referer", "https://y.qq.com/portal/player.html");

    auto *reply = nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleLyricReply(reply); });
  }

  void QQMusicClient::handleLyricReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit failed(reply->errorString());
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(stripJsonp(reply->readAll()), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
      emit notFound();
      return;
    }
    const auto obj = doc.object();
    if (obj.value("retcode").toInt() != 0) {
      emit notFound();
      return;
    }
    // The lyric field is base64-encoded LRC.
    const QString lyric = QString::fromUtf8(
      QByteArray::fromBase64(obj.value("lyric").toString().toLatin1())).trimmed();
    if (lyric.isEmpty()
        || (LrcParser::looksLikeLrc(lyric) && LrcParser::stripTimestamps(lyric).isEmpty())) {
      emit notFound();
      return;
    }
    emit found(lyric);
  }

  QByteArray QQMusicClient::stripJsonp(const QByteArray &body) {
    const QByteArray trimmed = body.trimmed();
    if (trimmed.startsWith('{') || trimmed.startsWith('[')) {
      return trimmed;
    }
    const int open = trimmed.indexOf('(');
    const int close = trimmed.lastIndexOf(')');
    if (open < 0 || close <= open) {
      return trimmed;
    }
    return trimmed.mid(open + 1, close - open - 1);
  }
}
