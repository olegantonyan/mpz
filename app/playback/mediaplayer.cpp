#include "mediaplayer.h"

#include <QBuffer>
#include <QFile>
#include <QThread>
#include <QtConcurrent>
#include <QEventLoop>
#include <QTimer>

namespace Playback {
  MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent) : QObject(parent), stream(stream_buffer_size), output_device_id(outdevid) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(&media_devices, &QMediaDevices::audioOutputsChanged, this, &MediaPlayer::onAudioDevicesChanged);
    player.setAudioOutput(&audio_output);
    setOutputDevice(output_device_id);
#endif
    connect(&player, &QMediaPlayer::positionChanged, [=](quint64 pos) {
      emit positionChanged(pos - offset_begin);
      if (offset_end > 0 && pos >= offset_end) {
        stop();
      }
    });
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(&player, &QMediaPlayer::playbackStateChanged, [=](QMediaPlayer::PlaybackState state) {
#else
    connect(&player, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
#endif
      switch (state) {
        case QMediaPlayer::StoppedState:
          emit positionChanged(0);
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(&player, &QMediaPlayer::errorOccurred, [=](QMediaPlayer::Error err) {
#else
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [=](QMediaPlayer::Error err) {
#endif
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    switch (player.playbackState()) {
#else
    switch (player.state()) {
#endif
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return audio_output.volume() * 100;
#else
    return player.volume();
#endif
  }

  qint64 MediaPlayer::position() const {
    return player.position() - offset_begin;
  }

  void MediaPlayer::pause() {
    player.pause();
  }

  void Playback::MediaPlayer::seek_to_offset_begin() {
    QTimer timer;
    QEventLoop loop;
    timer.setSingleShot(true);
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&player, &QMediaPlayer::seekableChanged, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    if (!player.isSeekable()) {
      qWarning() << "player is not seekable, setting offset begin will probably fail";
    }
    player.setPosition(offset_begin);
  }

  void MediaPlayer::play() {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (!stream.isRunning() && stream.isValidUrl()) {
      if (!stream.start()) {
        qWarning() << "error starting stream form" << stream.url();
      }
    }
#endif
    bool ff = state() == MediaPlayer::StoppedState; // prevent rewing when unpausing
    player.play();
    if (ff && offset_begin > 0) {
      seek_to_offset_begin();
    }
  }

  void MediaPlayer::stop() {
    player.stop();
    stream.stop();
  }

  void MediaPlayer::setPosition(qint64 pos) {
    if (player.isSeekable()) {
      player.setPosition(pos + offset_begin);
    }
  }

  void MediaPlayer::setVolume(int vol) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    audio_output.setVolume(vol / 100.0);
#else
    player.setVolume(vol);
#endif
  }

  void MediaPlayer::setTrack(const Track &track) {
    offset_begin = 0;
    offset_end = 0;
    stream.stop();
    if (track.isStream()) {
      stream.setUrl(track.url());
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      start_stream();
      player.setSourceDevice(&stream);
#else
      player.setMedia(track.url(), &stream);
#endif
    } else {

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      player.setSource(track.url());
#else
      player.setMedia(track.url());
#endif
      if (track.isCue()) {
        offset_begin = track.begin() * 1000;
        offset_end = offset_begin + track.duration() * 1000;
      }
    }
  }

  bool MediaPlayer::start_stream() {
    if (stream.isValidUrl()) {
      if (!stream.start()) {
        qWarning() << "error starting stream form" << stream.url();
        return false;
      }
    }
    QTimer timer;
    QEventLoop loop;
    timer.setSingleShot(true);
    timer.setInterval(3000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&stream, &Stream::readyRead, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    return stream.bytesAvailable() > 0;
  }

  void MediaPlayer::clearTrack() {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    player.setSource(QUrl(nullptr));
#else
    player.setMedia(nullptr);
#endif
    offset_begin = 0;
    offset_end = 0;
  }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  void MediaPlayer::setOutputDevice(QByteArray deviceid) {
    if (deviceid.isEmpty()) {
      audio_output.setDevice(QMediaDevices::defaultAudioOutput());
    } else {
      auto devices = QMediaDevices::audioOutputs();
      for (auto device : devices) {
        if (device.id() == deviceid) {
          audio_output.setDevice(device);
          break;
        }
      }
    }
  }

  void MediaPlayer::onAudioDevicesChanged() {
    auto devices = QMediaDevices::audioOutputs();
    for (auto device : devices) {
      if (device.id() == output_device_id) {
        audio_output.setDevice(QMediaDevices::defaultAudioOutput()); // hack to force device change if it was disconnected
        audio_output.setDevice(device);
        break;
      }
    }
  }
#endif
}
