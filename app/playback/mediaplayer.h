#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "playback/stream.h"
#include "track.h"

#include <QObject>
#include <QMediaPlayer>

namespace Playback {
  class MediaPlayer : public QObject {
    Q_OBJECT
  public:
    enum State {
      StoppedState,
      PlayingState,
      PausedState
    };

    explicit MediaPlayer(QObject *parent = nullptr);

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
  };
}

#endif // MEDIAPLAYER_H
