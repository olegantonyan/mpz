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
    Q_ENUM(State)

    explicit MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent = nullptr);

    virtual MediaPlayer::State state();
    virtual int volume();
    virtual qint64 position();

  signals:
    void positionChanged(qint64 position);
    void stateChanged(MediaPlayer::State newState);
    void error(const QString &message);
    void streamBufferfillChanged(quint32 current, quint32 total);
    void streamMetaChanged(const StreamMetaData& meta);
    void prevRequested();
    void nextRequested();

  public slots:
    virtual void pause();
    virtual void play();
    virtual void stop();
    virtual void next();
    virtual void prev();
    virtual void setPosition(qint64 position);
    virtual void setVolume(int volume);
    virtual void setTrack(const Track &track);
    virtual void clearTrack();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    virtual void setOutputDevice(QByteArray deviceid);
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
    bool unpause_workaround_needed_on_playing_state_change;

    quint64 offset_begin;
    quint64 offset_end;
    void seek_to_offset_begin();
    void unpause_workaround();
    void emitStateChanged(MediaPlayer::State state);

  private slots:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void onAudioDevicesChanged();
#endif
  };
}

#endif // MEDIAPLAYER_H
