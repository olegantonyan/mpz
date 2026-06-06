#ifndef LYRICS_PROVIDER_H
#define LYRICS_PROVIDER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

namespace Lyrics {
  struct TrackQuery {
    QString artist;
    QString title;
    QString album;
    int duration_seconds = 0; // 0 = unknown
  };

  // Base class for online lyrics providers. Exactly one of found/notFound/failed
  // is emitted per fetch().
  class Provider : public QObject {
    Q_OBJECT
  public:
    explicit Provider(QObject *parent = nullptr);

    virtual void fetch(const TrackQuery &query) = 0;

  signals:
    void found(const QString &lyrics);
    void notFound();
    void failed(const QString &message);

  protected:
    QNetworkRequest makeRequest(const QUrl &url, bool browser_like_ua = false) const;

    QNetworkAccessManager nam;
  };
}

#endif // LYRICS_PROVIDER_H
