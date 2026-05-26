#include "mediaplayer.h"

#include <QBuffer>
#include <QFile>
#include <QThread>
#include <QtConcurrent>
#include <QEventLoop>
#include <QTimer>

namespace Playback {
  MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent) : QObject(parent), stream(stream_buffer_size), output_device_id(outdevid), next_after_stop(true), next_after_stop_cue(true) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(&media_devices, &QMediaDevices::audioOutputsChanged, this, &MediaPlayer::onAudioDevicesChanged);
    player.setAudioOutput(&audio_output);
    setOutputDevice(output_device_id);
#endif
    connect(&player, &QMediaPlayer::positionChanged, this, [=](quint64 pos) {
      // pos can lag behind offset_begin briefly after a CUE setSource while
      // seek_to_offset_begin is still resolving; clamp to avoid unsigned
      // underflow that would publish a near-ULLONG_MAX duration to the UI.
      emit positionChanged(pos > offset_begin ? pos - offset_begin : 0);
      if (offset_end > 0 && pos >= offset_end) {
        // Soft CUE boundary: let the underlying file keep playing. If the
        // upcoming setTrack() points at the same file (sequential/random
        // same-file CUE) we'll skip setSource entirely; otherwise setSource
        // will halt playback as usual. next_after_stop is cleared so any
        // implicit StoppedState that setSource emits doesn't re-fire
        // nextRequested.
        offset_begin = offset_end;
        offset_end = 0;
        next_after_stop = false;
        emit positionChanged(0);
        QTimer::singleShot(0, this, [this]() { emit nextRequested(); });
      }
    });
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(&player, &QMediaPlayer::playbackStateChanged, this, [=](QMediaPlayer::PlaybackState state) {
#else
    connect(&player, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
#endif
      switch (state) {
        case QMediaPlayer::StoppedState:
          emit positionChanged(0);
          if (next_after_stop) {
            // Defer one tick so the next setSource/play doesn't re-enter
            // QMediaPlayer while it's still inside its own EOF emission.
            QTimer::singleShot(0, this, [this]() { emit nextRequested(); });
          } else {
            emitStateChanged(MediaPlayer::StoppedState);
          }
          break;
        case QMediaPlayer::PlayingState:
#ifdef QT6_STREAM_HACKS
          if (suppress_emit_playing_state) {
            suppress_emit_playing_state = false;
            break;
          }
#endif
          emitStateChanged(MediaPlayer::PlayingState);
          if (unpause_workaround_needed_on_playing_state_change) {
            unpause_workaround();
          }
          break;
        case QMediaPlayer::PausedState:
          emitStateChanged(MediaPlayer::PausedState);
          break;
      }
    });
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(&player, &QMediaPlayer::errorOccurred, this, [=](QMediaPlayer::Error err) {
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
#ifdef QT6_STREAM_HACKS
    suppress_emit_playing_state = false;
#endif
  }

  MediaPlayer::State MediaPlayer::state() {
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

  int MediaPlayer::volume() {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return audio_output.volume() * 100;
#else
    return player.volume();
#endif
  }

  qint64 MediaPlayer::position() {
    const qint64 pos = player.position();
    const qint64 begin = static_cast<qint64>(offset_begin);
    return pos > begin ? pos - begin : 0;
  }

  void MediaPlayer::pause() {
    if (state() == MediaPlayer::PausedState) {
      play();
    } else  {
      player.pause();
      unpause_workaround();
    }
  }

  void MediaPlayer::unpause_workaround() {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    //if (state() == MediaPlayer::PausedState) {
      setPosition(position() -1); // in Qt6 unpausing after a long pause leads to no sound until you seek or stop/start or change output
    //}
#endif
  }

  void MediaPlayer::emitStateChanged(State state) {
    if (state == PlayingState) {
      next_after_stop = true;
    }
    emit stateChanged(state);
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
    if (synthetic_playing_on_play) {
      synthetic_playing_on_play = false;
      next_after_stop = false;
      if (state() == MediaPlayer::PausedState) {
        // Paused mid-CUE then user picked a same-file track: actually resume.
        // QMediaPlayer will emit PlayingState; the normal handler emits
        // stateChanged(PlayingState).
        player.play();
        unpause_workaround();
      } else {
        // Already playing through a soft CUE boundary. QMediaPlayer wouldn't
        // emit a state transition, so synthesise the per-track PlayingState
        // event so downstream emits started(_current_track).
        emitStateChanged(MediaPlayer::PlayingState);
      }
      return;
    }
    next_after_stop = false;
#ifndef QT6_STREAM_HACKS
    if (!stream.isRunning() && stream.isValidUrl()) {
      if (!stream.start()) {
        qWarning() << "error starting stream form" << stream.url();
      }
    }
#endif
    bool ff = state() == MediaPlayer::StoppedState; // prevent rewing when unpausing
    player.play();
    if (ff && offset_begin > 0) {
      auto vol = volume();
      setVolume(0);
      seek_to_offset_begin();
      setVolume(vol);
    }
    unpause_workaround();
  }

  void MediaPlayer::stop() {
    next_after_stop = false;
    synthetic_playing_on_play = false;
    // Sequential-no-loop EOF: lambda emitted nextRequested, Dispatch routed
    // stop back here; player.stop() is a no-op so synthesise the transition.
    const bool already_stopped = state() == MediaPlayer::StoppedState;
    player.stop();
    stream.stop();
    if (already_stopped) {
      emitStateChanged(MediaPlayer::StoppedState);
    }
  }

  void MediaPlayer::next() {
    next_after_stop = false;
    emit nextRequested();
  }

  void MediaPlayer::prev() {
    next_after_stop = false;
    emit prevRequested();
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
    // A source swap is a deliberate action, never a real EOF. Clear the flag
    // before any setSource so the resulting StoppedState doesn't schedule a
    // spurious nextRequested (e.g. double-clicking a playlist would otherwise
    // skip to the 2nd track).
    next_after_stop = false;
    // Same-file CUE continuation: skip the expensive setSource/stream.stop
    // cycle. Works for sequential autoplay (player is already at the right
    // region) and for random/manual jumps inside the same file (cheap
    // in-file seek).
    const bool same_file_cue =
        track.isCue()
        && !track.isStream()
        && !current_source_url.isEmpty()
        && track.url() == current_source_url
        && state() != MediaPlayer::StoppedState;

    if (same_file_cue) {
      offset_begin = track.begin();
      offset_end = offset_begin + track.duration();
      const quint64 pos = player.position();
      if (pos < offset_begin || pos >= offset_end) {
        player.setPosition(offset_begin);
      }
      synthetic_playing_on_play = true;
      return;
    }

    offset_begin = 0;
    offset_end = 0;
    stream.stop();
    if (track.isStream()) {
      current_source_url = QUrl();
      stream.setUrl(track.url());
      unpause_workaround_needed_on_playing_state_change = false;
#ifdef QT6_STREAM_HACKS
      // optimistic state update b/c start_stream will block
      // also prevent double emit playing state after player starts playing
      suppress_emit_playing_state = true;
      emitStateChanged(MediaPlayer::PlayingState);
      if (start_stream()) {
        player.setSourceDevice(&stream);
      } else {
        emitStateChanged(MediaPlayer::StoppedState);
      }
#else
      player.setMedia(track.url(), &stream);
#endif
    } else {
      unpause_workaround_needed_on_playing_state_change = true;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      player.setSource(track.url());
#else
      player.setMedia(track.url());
#endif
      current_source_url = track.url();
      if (track.isCue()) {
        offset_begin = track.begin();
        offset_end = offset_begin + track.duration();
      }
    }
  }
#ifdef QT6_STREAM_HACKS
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
    timer.setInterval(30000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&stream, &Stream::readyRead, &loop, &QEventLoop::quit);
    connect(&stream, &Stream::error, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    return stream.bytesAvailable() > 0;
  }
#endif
  void MediaPlayer::clearTrack() {
    next_after_stop = false;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    player.setSource(QUrl(nullptr));
#else
    player.setMedia(nullptr);
#endif
    offset_begin = 0;
    offset_end = 0;
    current_source_url = QUrl();
    synthetic_playing_on_play = false;
  }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  void MediaPlayer::setOutputDevice(QByteArray deviceid) {
    if (deviceid.isEmpty()) {
      audio_output.setDevice(QMediaDevices::defaultAudioOutput());
    } else {
      auto devices = QMediaDevices::audioOutputs();
      for (const auto &device : std::as_const(devices)) {
        if (device.id() == deviceid) {
          audio_output.setDevice(device);
          break;
        }
      }
    }
  }

  void MediaPlayer::onAudioDevicesChanged() {
    auto devices = QMediaDevices::audioOutputs();
    for (const auto &device : std::as_const(devices)) {
      if (device.id() == output_device_id) {
        // hack to force device change if it was disconnected
        // desperation delay because sometimes it switches to default and keep playing through it
        audio_output.setDevice(QMediaDevices::defaultAudioOutput());
        QTimer timer;
        QEventLoop loop;
        timer.setSingleShot(true);
        timer.setInterval(100);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start();
        loop.exec();

        audio_output.setDevice(device);
        break;
      }
    }
  }
#endif
}
