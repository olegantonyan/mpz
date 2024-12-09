#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "playback/stream.h"
#include "track.h"

#include <QObject>
#include <QMediaPlayer>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #include <QAudioOutput>
  #include <QMediaDevices>
  #include <QAudioDevice>
  #define QT6_STREAM_HACKS
#endif

namespace Playback {
  class MediaPlayer : public QObject {
    Q_OBJECT
  public:
    enum State {
      StoppedState,
      PlayingState,
      PausedState
    };

    explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent = nullptr);

    MediaPlayer::State state() const;
    int volume() const;
    qint64 position() const;

  signals:
    void positionChanged(qint64 position);
    void stateChanged(MediaPlayer::State newState);
    void error(const QString &message);
    void streamBufferfillChanged(quint32 current, quint32 total);
    void streamMetaChanged(const StreamMetaData& meta);

  public slots:
    void pause();
    void play();
    void stop();
    void setPosition(qint64 position);
    void setVolume(int volume);
    void setTrack(const Track &track);
    void clearTrack();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void setOutputDevice(QByteArray deviceid);
#endif

  private:
    QMediaPlayer player;
    Stream stream;
    QByteArray output_device_id;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QAudioOutput audio_output;
    QMediaDevices media_devices;
#endif
#ifdef QT6_STREAM_HACKS
    bool suppress_emit_playing_state;
    bool start_stream();
#endif

    quint64 offset_begin;
    quint64 offset_end;
    void seek_to_offset_begin();
    void unpause_workaround();

  private slots:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void onAudioDevicesChanged();
#endif
  };
}

#endif // MEDIAPLAYER_H
