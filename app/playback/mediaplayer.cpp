#include "mediaplayer.h"

namespace Playback {
  MediaPlayer::MediaPlayer(QObject *parent) : QObject(parent) {
    connect(&player, &QMediaPlayer::positionChanged, this, &MediaPlayer::positionChanged);
    connect(&player, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
      switch (state) {
        case QMediaPlayer::StoppedState:
          emit stateChanged(MediaPlayer::StoppedState);
          break;
        case QMediaPlayer::PlayingState:
          emit stateChanged(MediaPlayer::PlayingState);
          break;
        case QMediaPlayer::PausedState:
          emit stateChanged(MediaPlayer::PausedState);
          break;
      }
    });
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [=](QMediaPlayer::Error err) {
      emit error(QString("QMediaPlayer error %1").arg(err));
    });
  }

  MediaPlayer::State MediaPlayer::state() const {
    switch (player.state()) {
      case QMediaPlayer::StoppedState:
        return MediaPlayer::StoppedState;
      case QMediaPlayer::PlayingState:
        return MediaPlayer::PlayingState;
      case QMediaPlayer::PausedState:
        return MediaPlayer::PausedState;
    }
  }

  int MediaPlayer::volume() const {
    return player.volume();
  }

  qint64 MediaPlayer::position() const {
    return player.position();
  }

  void MediaPlayer::pause() {
    player.pause();
  }

  void MediaPlayer::play() {
    player.play();
  }

  void MediaPlayer::stop() {
    player.stop();
  }

  void MediaPlayer::setPosition(qint64 pos) {
    player.setPosition(pos);
  }

  void MediaPlayer::setVolume(int vol) {
    player.setVolume(vol);
  }

  void MediaPlayer::setMedia(const QUrl &url) {
    player.setMedia(url);
  }

  void MediaPlayer::removeMedia() {
    player.setMedia(nullptr);
  }
}
