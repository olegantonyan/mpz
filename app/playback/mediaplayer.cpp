#include "mediaplayer.h"

#include <QBuffer>
#include <QFile>
#include <QThread>
#include <QtConcurrent>

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
    connect(&stream, &Stream::fillChanged, this, &MediaPlayer::streamBufferfillChanged);
    connect(&stream, &Stream::metadataChanged, this, &MediaPlayer::streamMetaChanged);
    connect(&stream, &Stream::error, [&](const QString& message) {
      qWarning() << "stream error" << message;
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
    return MediaPlayer::StoppedState;
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
    if (!stream.isRunning() && stream.isValidUrl()) {
      if (!stream.start()) {
        qWarning() << "error starting stream form" << stream.url();
      }
    }
    player.play();
  }

  void MediaPlayer::stop() {
    player.stop();
    stream.stop();
  }

  void MediaPlayer::setPosition(qint64 pos) {
    player.setPosition(pos);
  }

  void MediaPlayer::setVolume(int vol) {
    player.setVolume(vol);
  }

  void MediaPlayer::setTrack(const Track &track) {
    stream.stop();
    if (track.isStream()) {
      stream.setUrl(track.url());
      player.setMedia(track.url(), &stream);
    } else {
      player.setMedia(track.url());
    }
  }

  void MediaPlayer::clearTrack() {
    player.setMedia(nullptr);
  }
}
