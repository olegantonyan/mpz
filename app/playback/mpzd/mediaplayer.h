#ifndef MPZDMEDIAPLAYER_H
#define MPZDMEDIAPLAYER_H

#include "playback/mediaplayer.h"
#include "mpzd_client/client.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace Playback {
  namespace Mpzd {
    class MediaPlayer : public Playback::MediaPlayer {
      Q_OBJECT
    public:
      explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpzdClient::Client &cl, QObject *parent = nullptr);
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
      void trackLoaded(const Track &track);
      void volumeReported(int volume);

    private:
      MpzdClient::Client &client;
      Track current_track;
      bool track_changed = false;
      State current_state = StoppedState;
      quint64 current_uid = 0;
      quint32 base_pos_ms = 0;
      int last_volume = -1;
      QString last_icy;
      QElapsedTimer elapsed_clock;
      QTimer progress_timer;
      QTimer resync_timer;

      static State stateOf(const MpzdClient::Status &status);

    private slots:
      void onStatus(const MpzdClient::Status &status);
      void onDisconnected();
      void updateProgressNow();
    };
  }
}

#endif // MPZDMEDIAPLAYER_H
