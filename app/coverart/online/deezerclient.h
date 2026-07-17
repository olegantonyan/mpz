#ifndef COVERART_ONLINE_DEEZERCLIENT_H
#define COVERART_ONLINE_DEEZERCLIENT_H

#include "coverart/online/provider.h"

class QNetworkReply;

namespace CoverArt {
  namespace Online {
    // api.deezer.com album search. Keyless, one request, cover_xl is 1000x1000.
    class DeezerClient : public Provider {
      Q_OBJECT
    public:
      explicit DeezerClient(QObject *parent = nullptr);

      void fetch(const AlbumQuery &query) override;

    private:
      void handleSearchReply(QNetworkReply *reply);

      AlbumQuery query;
    };
  }
}

#endif // COVERART_ONLINE_DEEZERCLIENT_H
