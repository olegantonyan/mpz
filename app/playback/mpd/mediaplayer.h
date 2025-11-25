#ifndef MPDMEDIAPLAYER_H
#define MPDMEDIAPLAYER_H

#include "playback/mediaplayer.h"
#include "mpd_client/client.h"

#include <QObject>
#include <QUrl>

namespace Playback {
  namespace Mpd {
    class MediaPlayer : public Playback::MediaPlayer {
      Q_OBJECT
    public:
      explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdClient::Client &cl, QObject *parent = nullptr);

      // MediaPlayer interface
    public:
      MediaPlayer::State state() override;
      int volume() override;
      qint64 position() override;

    public slots:
      void pause() override;
      void play() override;
      void stop() override;
      void setPosition(qint64 position) override;
      void setVolume(int volume) override;
      void setTrack(const Track &track) override;
      void clearTrack() override;
      void next() override;
      void prev() override;

      void updateStatus();

    signals:
      void trackChanged(const QString &path);

    private:
      MpdClient::Client &client;
      Track current_track;
      unsigned int playing_song_id;

      QPair<unsigned int, QString> get_current_song();

    private slots:
      void on_playerStateChanged();
    };
  }
}

#endif // MPDMEDIAPLAYER_H
