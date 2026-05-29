#ifndef QTMM_MEDIAPLAYER_H
#define QTMM_MEDIAPLAYER_H

#include "playback/mediaplayer.h"
#include "playback/stream.h"

#include <QUrl>
#include <QMediaPlayer>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #include <QAudioOutput>
  #include <QMediaDevices>
  #include <QAudioDevice>
#endif

namespace Playback {
  namespace Qtmm {
    // QtMultimedia (QMediaPlayer) audio backend. Kept as a build-time fallback
    // behind ENABLE_QTMULTIMEDIA_BACKEND; the native FFmpeg engine is the
    // default. This is the original Playback::MediaPlayer implementation moved
    // verbatim out of the (now abstract) base class.
    class MediaPlayer : public Playback::MediaPlayer {
      Q_OBJECT
    public:
      explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent = nullptr);

      Playback::MediaPlayer::State state() override;
      int volume() override;
      qint64 position() override;
      QList<Playback::MediaPlayer::AudioDevice> audioOutputs() override;

    public slots:
      void pause() override;
      void play() override;
      void stop() override;
      void next() override;
      void prev() override;
      void setPosition(qint64 position) override;
      void setVolume(int volume) override;
      void setTrack(const Track &track) override;
      void clearTrack() override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      void setOutputDevice(QByteArray deviceid) override;
#endif

    private:
      QMediaPlayer player;
      Stream stream;
      QByteArray output_device_id;
      bool next_after_stop;
      bool next_after_stop_cue;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      QAudioOutput audio_output;
      QMediaDevices media_devices;
#endif
#ifdef QT6_STREAM_HACKS
      bool suppress_emit_playing_state;
      bool start_stream();
#endif
      bool unpause_workaround_needed_on_playing_state_change = false;

      quint64 offset_begin = 0;
      quint64 offset_end = 0;
      QUrl current_source_url;
      bool synthetic_playing_on_play = false;
      void seek_to_offset_begin();
      void unpause_workaround();
      void emitStateChanged(Playback::MediaPlayer::State state);

    private slots:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      void onAudioDevicesChanged();
#endif
    };
  }
}

#endif // QTMM_MEDIAPLAYER_H
