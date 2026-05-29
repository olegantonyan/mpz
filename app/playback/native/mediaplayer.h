#ifndef NATIVE_MEDIAPLAYER_H
#define NATIVE_MEDIAPLAYER_H

#include "playback/mediaplayer.h"
#include "playback/native/engine.h"

#include <QString>
#include <QTimer>

namespace Playback {
  namespace Native {
    // FFmpeg + miniaudio audio backend. A thin Qt wrapper around the Qt-free
    // Native::Engine: it maps the engine's absolute-time playback to the
    // Playback::MediaPlayer contract, layers CUE sub-track windowing on top
    // (gaplessly), and marshals the engine's decode-thread callbacks onto the
    // GUI thread.
    class MediaPlayer : public Playback::MediaPlayer {
      Q_OBJECT
    public:
      explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent = nullptr);
      ~MediaPlayer() override;

      Playback::MediaPlayer::State state() override;
      int volume() override;
      qint64 position() override;
      QList<Playback::MediaPlayer::AudioDevice> audioOutputs() override;

    public slots:
      void pause() override;
      void play() override;
      void stop() override;
      void setPosition(qint64 position) override;
      void setVolume(int volume) override;
      void setTrack(const Track &track) override;
      void clearTrack() override;
      void setOutputDevice(QByteArray deviceid) override;

    private:
      Engine engine;
      QTimer position_timer;
      int volume_pct = 100;
      QByteArray output_device_id;

      // CUE windowing over the engine's absolute-time playback (mirrors the
      // QtMultimedia backend's soft-boundary handling, but gapless because the
      // engine never stops at a sub-track boundary).
      QString current_source_path;
      quint64 offset_begin = 0;
      quint64 offset_end = 0;          // 0 == no sub-track end (play to natural EOF)
      bool synthetic_play = false;     // same-file CUE continuation awaiting play()
      bool pending_boundary_next = false;

      void onEos();

    private slots:
      void emitPosition();
    };
  } // namespace Native
} // namespace Playback

#endif // NATIVE_MEDIAPLAYER_H
