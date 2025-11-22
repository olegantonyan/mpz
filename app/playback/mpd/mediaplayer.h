#ifndef MPDMEDIAPLAYER_H
#define MPDMEDIAPLAYER_H

#include "playback/mediaplayer.h"
#include "mpdconnection.h"

#include <QObject>
#include <QUrl>

namespace Playback {
  namespace Mpd {
    class MediaPlayer : public Playback::MediaPlayer {
      Q_OBJECT
    public:
      explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdConnection &conn, QObject *parent = nullptr);

      // MediaPlayer interface
    public:
      MediaPlayer::State state() const override;
      int volume() const override;
      qint64 position() const override;

    public slots:
      void pause() override;
      void play() override;
      void stop() override;
      void setPosition(qint64 position) override;
      void setVolume(int volume) override;
      void setTrack(const Track &track) override;
      void clearTrack() override;
      void setOutputDevice(QByteArray deviceid) override;

      void updateStatus();

    private:
      MpdConnection &connection;
      Track current_track;

    private slots:
      void on_connected(const QUrl &url);
    };
  }
}

#endif // MPDMEDIAPLAYER_H
