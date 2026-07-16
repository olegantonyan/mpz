#include "coverart/online/deezerclient.h"
#include "coverart/online/albummatch.h"
#include "lyrics/textmatch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

namespace CoverArt {
  namespace Online {
    DeezerClient::DeezerClient(QObject *parent) : Provider(parent) {
    }

    void DeezerClient::fetch(const AlbumQuery &q) {
      query = q;
      const QString artist = Lyrics::TextMatch::normalizeArtist(q.artist);
      const QString album = Lyrics::TextMatch::normalizeTitle(q.album);
      if (artist.isEmpty() || album.isEmpty()) {
        emitNotFound();
        return;
      }

      QUrl url(QStringLiteral("https://api.deezer.com/search/album"));
      QUrlQuery params;
      params.addQueryItem("q", QString("artist:\"%1\" album:\"%2\"").arg(artist, album));
      params.addQueryItem("limit", "5");
      url.setQuery(params);

      auto *reply = nam.get(makeRequest(url));
      connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSearchReply(reply); });
    }

    void DeezerClient::handleSearchReply(QNetworkReply *reply) {
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
      const auto data = doc.object().value("data").toArray();

      QVector<AlbumCandidate> candidates;
      QStringList covers;
      for (const auto &entry : data) {
        const auto obj = entry.toObject();
        const QString cover = obj.value("cover_xl").toString();
        if (cover.isEmpty()) {
          continue;
        }
        candidates.append({obj.value("artist").toObject().value("name").toString(),
                           obj.value("title").toString()});
        covers << cover;
      }

      const int best = AlbumMatch::bestCandidate(candidates, query.artist, query.album);
      if (best < 0) {
        emitNotFound();
        return;
      }
      downloadImage(QUrl(covers.at(best)));
    }
  }
}
