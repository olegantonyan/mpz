#include "coverart/online/downloader.h"
#include "coverart/online/cache.h"
#include "coverart/online/providerchain.h"
#include "config/global.h"

namespace CoverArt {
  namespace Online {
    Downloader &Downloader::instance() {
      static Downloader self;
      return self;
    }

    Downloader::Downloader(QObject *parent) : QObject(parent) {
    }

    void Downloader::request(const Track &track) {
      if (track.isStream()) {
        return;
      }
      const AlbumQuery query{track.artist(), track.album()};
      if (query.artist.isEmpty() || query.album.isEmpty()) {
        return;
      }
      const QString key = Cache::key(query);
      if (key.isEmpty() || in_flight.contains(key)) {
        return;
      }
      // A built-in source or an earlier download already covers this track.
      if (!track.artCover().isEmpty()) {
        return;
      }
      Config::Global global;
      const auto providers = ProviderChain::filterKnown(global.coverProviders());
      if (providers.isEmpty()) {
        return;
      }
      in_flight.insert(key);
      emit searchStarted(query.artist, query.album);
      start(query, providers);
    }

    bool Downloader::isSearching(const QString &artist, const QString &album) const {
      const QString key = Cache::key(AlbumQuery{artist, album});
      return !key.isEmpty() && in_flight.contains(key);
    }

    void Downloader::start(const AlbumQuery &query, const QStringList &providers) {
      const QString key = Cache::key(query);
      auto *chain = new ProviderChain(this);
      connect(chain, &ProviderChain::found, this, [this, chain, key, query](const QString &, const QString &path) {
        in_flight.remove(key);
        chain->deleteLater();
        emit coverAvailable(query.artist, query.album, path);
      });
      connect(chain, &ProviderChain::notFound, this, [this, chain, key, query]() {
        in_flight.remove(key);
        chain->deleteLater();
        emit searchFinished(query.artist, query.album);
      });
      chain->fetch(providers, query);
    }
  }
}
