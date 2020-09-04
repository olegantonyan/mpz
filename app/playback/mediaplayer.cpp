#include "mediaplayer.h"

#include <QBuffer>
#include <QFile>
#include <QThread>
#include <QtConcurrent>

namespace Playback {
  MediaPlayer::MediaPlayer(QObject *parent) : QObject(parent) {
    connect(&player, &QMediaPlayer::positionChanged, [=](quint64 pos) {
      emit positionChanged(pos - offset_begin);
      if (offset_end > 0 && pos >= offset_end) {
        stop();
      }
    });
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
    offset_begin = 0;
    offset_end = 0;
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
    return player.position() - offset_begin;
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
    bool ff = state() == MediaPlayer::StoppedState; // prevent rewing when unpausing
    player.play();
    if (ff && offset_begin > 0) {
      player.setPosition(offset_begin);
    }
  }

  void MediaPlayer::stop() {
    player.stop();
    stream.stop();
  }

  void MediaPlayer::setPosition(qint64 pos) {
    player.setPosition(pos + offset_begin);
  }

  void MediaPlayer::setVolume(int vol) {
    player.setVolume(vol);
  }

  void MediaPlayer::setTrack(const Track &track) {
    offset_begin = 0;
    offset_end = 0;
    stream.stop();
    if (track.isStream()) {
      stream.setUrl(track.url());
      player.setMedia(track.url(), &stream);
    } else {
      player.setMedia(track.url());
      if (track.isCue()) {
        offset_begin = track.begin() * 1000;
        offset_end = offset_begin + track.duration() * 1000;
      }
    }
  }

  void MediaPlayer::clearTrack() {
    player.setMedia(nullptr);
    offset_begin = 0;
    offset_end = 0;
  }
}
