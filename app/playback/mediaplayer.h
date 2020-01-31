#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

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
  };
}

#endif // MEDIAPLAYER_H
