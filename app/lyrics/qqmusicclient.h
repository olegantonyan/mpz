#ifndef LYRICS_QQMUSICCLIENT_H
#define LYRICS_QQMUSICCLIENT_H

#include "lyrics/provider.h"

#include <QByteArray>
#include <QString>

class QNetworkReply;

namespace Lyrics {
  // QQ Music (y.qq.com). Unofficial API, no key. Search results carry no
  // duration, so scoring falls back to title/artist only.
  class QQMusicClient : public Provider {
    Q_OBJECT
  public:
    explicit QQMusicClient(QObject *parent = nullptr);

    void fetch(const TrackQuery &query) override;

  private:
    void handleSearchReply(QNetworkReply *reply);
    void fetchLyric(const QString &song_mid);
    void handleLyricReply(QNetworkReply *reply);
    static QByteArray stripJsonp(const QByteArray &body);

    TrackQuery query;
  };
}

#endif // LYRICS_QQMUSICCLIENT_H
