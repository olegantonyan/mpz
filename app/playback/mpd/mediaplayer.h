#ifndef MPDMEDIAPLAYER_H
#define MPDMEDIAPLAYER_H

#include "playback/mediaplayer.h"
#include "mpd_client/client.h"

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QElapsedTimer>

namespace Playback {
  namespace Mpd {
    class MediaPlayer : public Playback::MediaPlayer {
      Q_OBJECT
    public:
      explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdClient::Client &cl, QObject *parent = nullptr);
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

    signals:
      void trackChanged(const QString &path);
      void audioFormatUpdated(quint16 sample_rate, quint8 channels, quint16 bitrate);
      void durationChanged(quint64 ms);

    private:
      MpdClient::Client &client;
      Track current_track;
      MpdClient::Song last_song;
      QElapsedTimer elapsed_clock;
      QTimer progress_timer;
      MpdClient::Status last_status;

      MediaPlayer::State stateByStatus(const MpdClient::Status &status);

    private slots:
      void on_playerStateChanged();
      void updateProgressNow();
    };
  }
}

#endif // MPDMEDIAPLAYER_H
