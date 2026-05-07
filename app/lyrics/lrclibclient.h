#ifndef LYRICS_LRCLIBCLIENT_H
#define LYRICS_LRCLIBCLIENT_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

namespace Lyrics {
  class LrcLibClient : public QObject {
    Q_OBJECT
  public:
    explicit LrcLibClient(QObject *parent = nullptr);

    void fetch(const QString &artist, const QString &title, const QString &album, int duration_seconds);

  signals:
    void found(const QString &lyrics);
    void notFound();
    void failed(const QString &message);

  private:
    void searchFallback(const QString &artist, const QString &title);

    QNetworkAccessManager nam;
  };
}

#endif // LYRICS_LRCLIBCLIENT_H
