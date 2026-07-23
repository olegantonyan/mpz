#ifndef COVERART_ONLINE_COVERARTARCHIVECLIENT_H
#define COVERART_ONLINE_COVERARTARCHIVECLIENT_H

#include "coverart/online/provider.h"

class QNetworkReply;

namespace CoverArt {
  namespace Online {
    // MusicBrainz release search followed by a Cover Art Archive front image.
    // Keyless but the slowest provider: two round-trips, a redirect to
    // archive.org, and MusicBrainz's 1 req/sec limit.
    class CoverArtArchiveClient : public Provider {
      Q_OBJECT
    public:
      explicit CoverArtArchiveClient(QObject *parent = nullptr);

      void fetch(const AlbumQuery &query) override;

    private:
      void searchMusicBrainz();
      void handleSearchReply(QNetworkReply *reply);

      AlbumQuery query;
    };
  }
}

#endif // COVERART_ONLINE_COVERARTARCHIVECLIENT_H
