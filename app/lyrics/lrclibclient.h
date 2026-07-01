#ifndef LYRICS_LRCLIBCLIENT_H
#define LYRICS_LRCLIBCLIENT_H

#include "lyrics/provider.h"

#include <QString>
#include <QVector>

class QNetworkReply;

namespace Lyrics {
  // lrclib.net client. Runs a cascade of progressively looser queries and
  // stops at the first confident hit; search results are ranked with
  // TextMatch::bestCandidate instead of first-match-wins.
  class LrcLibClient : public Provider {
    Q_OBJECT
  public:
    explicit LrcLibClient(QObject *parent = nullptr);

    void fetch(const TrackQuery &query) override;

  private:
    struct Attempt {
      enum class Kind { Get, Search, SearchQ };
      Kind kind = Kind::Get;
      QString artist;
      QString title;
      QString album;
      int duration_seconds = 0;
    };

    void addSearchAttempt(const QString &artist, const QString &title);
    void runNext();
    void handleGetReply(QNetworkReply *reply);
    void handleSearchReply(QNetworkReply *reply);

    QVector<Attempt> attempts;
    int next_attempt = 0;
    TrackQuery query;
    QString last_error;
  };
}

#endif // LYRICS_LRCLIBCLIENT_H
