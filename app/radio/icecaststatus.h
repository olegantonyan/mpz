#ifndef RADIO_ICECASTSTATUS_H
#define RADIO_ICECASTSTATUS_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

namespace Radio {
  // Reads title, display-title, metadata.x_icy_title, then the last playlist.trackList
  // entry. *source_found stays false when the response carries no source at all.
  QString nowPlayingFromStatusJson(const QByteArray &body, const QString &mount, bool *source_found = nullptr);

  QUrl statusJsonUrl(const QUrl &stream_url);

  // Polls a station's status-json.xsl for stations whose inline ICY StreamTitle is
  // empty or absent. Start it on stream playback, stop it once inline metadata works.
  class IcecastStatus : public QObject {
    Q_OBJECT
  public:
    explicit IcecastStatus(QObject *parent = nullptr);

    void start(const QUrl &stream_url);
    void stop();

  signals:
    void nowPlaying(const QString &raw);

  private:
    void poll();
    void handleReply();

    QNetworkAccessManager nam;
    QTimer timer;
    QTimer request_timeout;
    QNetworkReply *current;
    QUrl status_url;
    QString mount;
    QByteArray auth_header;
    QString last_emitted;
    bool retried_without_mount;
  };
}

#endif // RADIO_ICECASTSTATUS_H
