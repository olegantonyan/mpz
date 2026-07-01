#ifndef LYRICS_NETEASECLIENT_H
#define LYRICS_NETEASECLIENT_H

#include "lyrics/provider.h"

#include <QString>

class QNetworkReply;

namespace Lyrics {
  // NetEase Cloud Music (music.163.com). Unofficial API, no key. Search has
  // real fuzzy matching, so it often rescues misspelled tags lrclib can't.
  class NetEaseClient : public Provider {
    Q_OBJECT
  public:
    explicit NetEaseClient(QObject *parent = nullptr);

    void fetch(const TrackQuery &query) override;

  private:
    void handleSearchReply(QNetworkReply *reply);
    void fetchLyric(qint64 song_id);
    void handleLyricReply(QNetworkReply *reply);

    TrackQuery query;
  };
}

#endif // LYRICS_NETEASECLIENT_H
