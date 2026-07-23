#include "coverart/online/coverartarchiveclient.h"
#include "coverart/online/albummatch.h"
#include "lyrics/textmatch.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>

namespace CoverArt {
  namespace Online {
    namespace {
      const qint64 MB_MIN_INTERVAL_MS = 1100; // musicbrainz.org allows ~1 req/sec

      QElapsedTimer &clock() {
        static QElapsedTimer t;
        if (!t.isValid()) {
          t.start();
        }
        return t;
      }

      // The throttle has to be process-global: ProviderChain builds a fresh
      // client per fetch and destroys it after, so an instance member would
      // never see the previous request. Reserving the slot at scheduling time
      // (rather than at send time) keeps concurrent callers staggered.
      qint64 reserveMusicBrainzSlot() {
        static qint64 next_allowed = 0;
        const qint64 now = clock().elapsed();
        const qint64 send_at = qMax(now, next_allowed);
        next_allowed = send_at + MB_MIN_INTERVAL_MS;
        return send_at - now;
      }
    }

    CoverArtArchiveClient::CoverArtArchiveClient(QObject *parent) : Provider(parent) {
    }

    void CoverArtArchiveClient::fetch(const AlbumQuery &q) {
      query = q;
      if (Lyrics::TextMatch::normalizeArtist(q.artist).isEmpty() ||
          Lyrics::TextMatch::normalizeTitle(q.album).isEmpty()) {
        emitNotFound();
        return;
      }
      const qint64 delay = reserveMusicBrainzSlot();
      // Context object is `this`, so a destroyed provider never fires.
      QTimer::singleShot(static_cast<int>(delay), this, [this]() { searchMusicBrainz(); });
    }

    void CoverArtArchiveClient::searchMusicBrainz() {
      const QString artist = Lyrics::TextMatch::normalizeArtist(query.artist);
      const QString album = Lyrics::TextMatch::normalizeTitle(query.album);

      QUrl url(QStringLiteral("https://musicbrainz.org/ws/2/release/"));
      QUrlQuery params;
      params.addQueryItem("query", QString("artist:\"%1\" AND release:\"%2\"").arg(artist, album));
      params.addQueryItem("fmt", "json");
      params.addQueryItem("limit", "5");
      url.setQuery(params);

      // MusicBrainz rejects user agents without contact information.
      auto *reply = nam.get(makeRequest(url, Ua::Contact));
      connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSearchReply(reply); });
    }

    void CoverArtArchiveClient::handleSearchReply(QNetworkReply *reply) {
      reply->deleteLater();
      if (reply->error() != QNetworkReply::NoError) {
        emitFailed(reply->errorString());
        return;
      }
      QJsonParseError perr;
      const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
      if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        emitNotFound();
        return;
      }
      const auto releases = doc.object().value("releases").toArray();

      QVector<AlbumCandidate> candidates;
      QStringList mbids;
      for (const auto &entry : releases) {
        const auto obj = entry.toObject();
        const QString mbid = obj.value("id").toString();
        if (mbid.isEmpty()) {
          continue;
        }
        QString artist;
        const auto credits = obj.value("artist-credit").toArray();
        if (!credits.isEmpty()) {
          artist = credits.first().toObject().value("name").toString();
        }
        candidates.append({artist, obj.value("title").toString()});
        mbids << mbid;
      }

      const int best = AlbumMatch::bestCandidate(candidates, query.artist, query.album);
      if (best < 0) {
        emitNotFound();
        return;
      }
      // Answers 404 when the release has no art, and 307 to archive.org when it
      // does — the base class sets a redirect policy so Qt5 follows it too.
      downloadImage(QUrl(QString("https://coverartarchive.org/release/%1/front-500").arg(mbids.at(best))));
    }
  }
}
