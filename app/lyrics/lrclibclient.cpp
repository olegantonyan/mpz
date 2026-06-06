#include "lyrics/lrclibclient.h"
#include "lyrics/textmatch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

namespace Lyrics {
  LrcLibClient::LrcLibClient(QObject *parent) : Provider(parent) {
  }

  void LrcLibClient::fetch(const TrackQuery &q) {
    query = q;
    attempts.clear();
    next_attempt = 0;
    last_error.clear();

    const QString norm_title = TextMatch::normalizeTitle(q.title);
    const QString norm_artist = TextMatch::normalizeArtist(q.artist);
    const QString prim_artist = TextMatch::normalizeArtist(TextMatch::primaryArtist(q.artist));

    // 1. Exact signature. Sent duration is matched server-side within ±2s, so
    //    an off-by-a-few-seconds tag silently misses — attempt 2 rescues that.
    if (!q.album.isEmpty() || q.duration_seconds > 0) {
      attempts.append({Attempt::Kind::Get, q.artist, q.title, q.album, q.duration_seconds});
    }
    // 2. Get without album/duration (the API is lenient about the "required" fields).
    attempts.append({Attempt::Kind::Get, q.artist, q.title, QString(), 0});
    // 3-5. Field search: raw, normalized, primary artist. lrclib search has no
    //      typo tolerance, so stripping decorations client-side is what helps.
    addSearchAttempt(q.artist, q.title);
    addSearchAttempt(norm_artist, norm_title);
    addSearchAttempt(prim_artist, norm_title);
    // 6. General full-text query as the loosest fallback.
    Attempt qa;
    qa.kind = Attempt::Kind::SearchQ;
    qa.artist = norm_artist;
    qa.title = norm_title;
    attempts.append(qa);

    runNext();
  }

  void LrcLibClient::addSearchAttempt(const QString &artist, const QString &title) {
    if (title.isEmpty()) {
      return;
    }
    for (const auto &a : attempts) {
      if (a.kind == Attempt::Kind::Search && a.artist == artist && a.title == title) {
        return;
      }
    }
    Attempt a;
    a.kind = Attempt::Kind::Search;
    a.artist = artist;
    a.title = title;
    attempts.append(a);
  }

  void LrcLibClient::runNext() {
    if (next_attempt >= attempts.size()) {
      if (!last_error.isEmpty()) {
        emit failed(last_error);
      } else {
        emit notFound();
      }
      return;
    }
    const Attempt attempt = attempts.at(next_attempt++);

    QUrl url;
    QUrlQuery uq;
    if (attempt.kind == Attempt::Kind::Get) {
      url = QUrl("https://lrclib.net/api/get");
      uq.addQueryItem("artist_name", attempt.artist);
      uq.addQueryItem("track_name", attempt.title);
      if (!attempt.album.isEmpty()) {
        uq.addQueryItem("album_name", attempt.album);
      }
      if (attempt.duration_seconds > 0) {
        uq.addQueryItem("duration", QString::number(attempt.duration_seconds));
      }
    } else {
      url = QUrl("https://lrclib.net/api/search");
      if (attempt.kind == Attempt::Kind::SearchQ) {
        const QString q = attempt.artist.isEmpty() ? attempt.title : attempt.artist + " " + attempt.title;
        uq.addQueryItem("q", q);
      } else {
        if (!attempt.artist.isEmpty()) {
          uq.addQueryItem("artist_name", attempt.artist);
        }
        uq.addQueryItem("track_name", attempt.title);
      }
    }
    url.setQuery(uq);

    auto *reply = nam.get(makeRequest(url));
    if (attempt.kind == Attempt::Kind::Get) {
      connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleGetReply(reply); });
    } else {
      connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSearchReply(reply); });
    }
  }

  void LrcLibClient::handleGetReply(QNetworkReply *reply) {
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::ContentNotFoundError || status == 404) {
      runNext();
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      last_error = reply->errorString();
      runNext();
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
      last_error = QString("invalid response from lrclib");
      runNext();
      return;
    }
    const auto obj = doc.object();
    if (obj.value("instrumental").toBool()) {
      runNext();
      return;
    }
    const QString synced = obj.value("syncedLyrics").toString();
    const QString plain = obj.value("plainLyrics").toString();
    const QString result = !synced.isEmpty() ? synced : plain;
    if (result.isEmpty()) {
      runNext();
      return;
    }
    emit found(result);
  }

  void LrcLibClient::handleSearchReply(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      last_error = reply->errorString();
      runNext();
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isArray()) {
      runNext();
      return;
    }
    QVector<SongCandidate> candidates;
    QVector<QString> lyrics; // candidate index -> lyrics text
    const auto arr = doc.array();
    for (const auto &v : arr) {
      if (!v.isObject()) {
        continue;
      }
      const auto obj = v.toObject();
      const QString synced = obj.value("syncedLyrics").toString();
      const QString plain = obj.value("plainLyrics").toString();
      const QString text = !synced.isEmpty() ? synced : plain;
      if (text.isEmpty()) {
        continue;
      }
      SongCandidate c;
      c.title = obj.value("trackName").toString();
      c.artist = obj.value("artistName").toString();
      c.duration_seconds = static_cast<int>(obj.value("duration").toDouble());
      c.has_synced = !synced.isEmpty();
      c.instrumental = obj.value("instrumental").toBool();
      candidates.append(c);
      lyrics.append(text);
    }
    const int best = TextMatch::bestCandidate(candidates, query.artist, query.title, query.duration_seconds);
    if (best >= 0) {
      emit found(lyrics.at(best));
      return;
    }
    runNext();
  }
}
