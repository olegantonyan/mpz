#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "playback/stream.h"

#include <QObject>
#include <QMediaPlayer>
#include <QProcess>

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
    void setMedia(const QUrl &url);
    void removeMedia();

  private:
    QMediaPlayer player;
    Stream stream;
    QProcess ffplay;
  };
}

#endif // MEDIAPLAYER_H
