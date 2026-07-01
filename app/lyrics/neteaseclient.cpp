#include "lyrics/neteaseclient.h"
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
  NetEaseClient::NetEaseClient(QObject *parent) : Provider(parent) {
  }

  void NetEaseClient::fetch(const TrackQuery &q) {
    query = q;

    const QString artist = TextMatch::normalizeArtist(q.artist);
    const QString title = TextMatch::normalizeTitle(q.title);
    const QString s = artist.isEmpty() ? title : artist + " " + title;

    QUrlQuery body;
    body.addQueryItem("s", s);
    body.addQueryItem("type", "1");
    body.addQueryItem("limit", "10");
    body.addQueryItem("offset", "0");

    auto req = makeRequest(QUrl("https://music.163.com/api/cloudsearch/pc"), true);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setRawHeader("Referer", "https://music.163.com/");

    auto *reply = nam.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSearchReply(reply); });
  }

  void NetEaseClient::handleSearchReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit failed(reply->errorString());
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
      emit notFound();
      return;
    }
    const auto obj = doc.object();
    if (obj.value("code").toInt() != 200) {
      emit notFound();
      return;
    }
    QVector<SongCandidate> candidates;
    QVector<qint64> ids; // candidate index -> song id
    const auto songs = obj.value("result").toObject().value("songs").toArray();
    for (const auto &v : songs) {
      if (!v.isObject()) {
        continue;
      }
      const auto song = v.toObject();
      const qint64 id = static_cast<qint64>(song.value("id").toDouble());
      if (id <= 0) {
        continue;
      }
      QStringList artists;
      const auto ar = song.value("ar").toArray();
      for (const auto &a : ar) {
        const QString name = a.toObject().value("name").toString();
        if (!name.isEmpty()) {
          artists << name;
        }
      }
      SongCandidate c;
      c.title = song.value("name").toString();
      c.artist = artists.join(", ");
      c.duration_seconds = static_cast<int>(song.value("dt").toDouble() / 1000.0); // dt is ms
      candidates.append(c);
      ids.append(id);
    }
    const int best = TextMatch::bestCandidate(candidates, query.artist, query.title, query.duration_seconds);
    if (best < 0) {
      emit notFound();
      return;
    }
    fetchLyric(ids.at(best));
  }

  void NetEaseClient::fetchLyric(qint64 song_id) {
    QUrlQuery body;
    body.addQueryItem("id", QString::number(song_id));
    body.addQueryItem("os", "pc");
    body.addQueryItem("lv", "-1");
    body.addQueryItem("kv", "-1");
    body.addQueryItem("tv", "-1");

    auto req = makeRequest(QUrl("https://interface3.music.163.com/api/song/lyric"), true);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setRawHeader("Referer", "https://music.163.com/");

    auto *reply = nam.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleLyricReply(reply); });
  }

  void NetEaseClient::handleLyricReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit failed(reply->errorString());
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
      emit notFound();
      return;
    }
    const auto obj = doc.object();
    if (obj.value("nolyric").toBool() || obj.value("uncollected").toBool()) {
      emit notFound();
      return;
    }
    const QString lyric = obj.value("lrc").toObject().value("lyric").toString().trimmed();
    // Reject empty or metadata-only LRC bodies.
    if (lyric.isEmpty()
        || (LrcParser::looksLikeLrc(lyric) && LrcParser::stripTimestamps(lyric).isEmpty())) {
      emit notFound();
      return;
    }
    emit found(lyric);
  }
}
