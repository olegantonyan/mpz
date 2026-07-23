#include "coverart/online/itunesclient.h"
#include "coverart/online/albummatch.h"
#include "lyrics/textmatch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

namespace CoverArt {
  namespace Online {
    ItunesClient::ItunesClient(QObject *parent) : Provider(parent) {
    }

    void ItunesClient::fetch(const AlbumQuery &q) {
      query = q;
      const QString artist = Lyrics::TextMatch::normalizeArtist(q.artist);
      const QString album = Lyrics::TextMatch::normalizeTitle(q.album);
      if (artist.isEmpty() || album.isEmpty()) {
        emitNotFound();
        return;
      }

      QUrl url(QStringLiteral("https://itunes.apple.com/search"));
      QUrlQuery params;
      params.addQueryItem("term", artist + " " + album);
      params.addQueryItem("entity", "album");
      params.addQueryItem("limit", "5");
      url.setQuery(params);

      auto *reply = nam.get(makeRequest(url));
      connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSearchReply(reply); });
    }

    void ItunesClient::handleSearchReply(QNetworkReply *reply) {
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
      const auto results = doc.object().value("results").toArray();

      QVector<AlbumCandidate> candidates;
      QStringList covers;
      for (const auto &entry : results) {
        const auto obj = entry.toObject();
        QString art = obj.value("artworkUrl100").toString();
        if (art.isEmpty()) {
          continue;
        }
        // The search API only ever advertises 100x100, but the CDN serves any
        // size from the same path.
        art.replace(QStringLiteral("100x100bb"), QStringLiteral("600x600bb"));
        candidates.append({obj.value("artistName").toString(),
                           obj.value("collectionName").toString()});
        covers << art;
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
