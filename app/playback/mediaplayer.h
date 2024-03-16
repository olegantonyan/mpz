#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "playback/stream.h"
#include "track.h"

#include <QObject>
#include <QMediaPlayer>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #include <QAudioOutput>
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

    explicit MediaPlayer(quint32 stream_buffer_size, QObject *parent = nullptr);

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

  private:
    QMediaPlayer player;
    Stream stream;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QAudioOutput audio_output;
#endif

    quint64 offset_begin;
    quint64 offset_end;
    void seek_to_offset_begin();
  };
}

#endif // MEDIAPLAYER_H
