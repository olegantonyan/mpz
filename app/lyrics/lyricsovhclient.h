#ifndef LYRICS_LYRICSOVHCLIENT_H
#define LYRICS_LYRICSOVHCLIENT_H

#include "lyrics/provider.h"

#include <QString>
#include <QVector>

class QNetworkReply;

namespace Lyrics {
  // lyrics.ovh: trivial keyless API, plain text only. Last-resort fallback.
  class LyricsOvhClient : public Provider {
    Q_OBJECT
  public:
    explicit LyricsOvhClient(QObject *parent = nullptr);

    void fetch(const TrackQuery &query) override;

  private:
    struct Attempt {
      QString artist;
      QString title;
    };

    void runNext();
    void handleReply(QNetworkReply *reply);

    QVector<Attempt> attempts;
    int next_attempt = 0;
    QString last_error;
  };
}

#endif // LYRICS_LYRICSOVHCLIENT_H
