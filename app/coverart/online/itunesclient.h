#ifndef COVERART_ONLINE_ITUNESCLIENT_H
#define COVERART_ONLINE_ITUNESCLIENT_H

#include "coverart/online/provider.h"

class QNetworkReply;

namespace CoverArt {
  namespace Online {
    // itunes.apple.com search. Keyless, one request; the 100x100 artwork URL it
    // returns is rewritten to a larger size before download.
    class ItunesClient : public Provider {
      Q_OBJECT
    public:
      explicit ItunesClient(QObject *parent = nullptr);

      void fetch(const AlbumQuery &query) override;

    private:
      void handleSearchReply(QNetworkReply *reply);

      AlbumQuery query;
    };
  }
}

#endif // COVERART_ONLINE_ITUNESCLIENT_H
