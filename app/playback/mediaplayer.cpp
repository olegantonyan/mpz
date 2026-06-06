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
    devices_changed_debounce.setSingleShot(true);
    devices_changed_debounce.setInterval(200); // unplug/replug emits audioOutputsChanged in bursts; coalesce into one evaluation
    connect(&devices_changed_debounce, &QTimer::timeout, this, &MediaPlayer::evaluateAudioDevice);
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
    output_device_id = deviceid;
    preferred_device_missing = false;
    ++device_change_epoch; // invalidate in-flight device-switch timers
    if (deviceid.isEmpty()) {
      // the ffmpeg backend resolves the sink at open time and doesn't follow
      // system default changes (a null QAudioDevice doesn't either); pin the
      // current default and re-pin in evaluateAudioDevice when it changes
      audio_output.setDevice(QMediaDevices::defaultAudioOutput());
      return;
    }
    const QAudioDevice device = findPreferredDevice();
    if (device.isNull()) {
      // configured device not present (e.g. unplugged before startup): fall back
      // to the default; evaluateAudioDevice switches back when it appears
      preferred_device_missing = true;
      audio_output.setDevice(QMediaDevices::defaultAudioOutput());
    } else {
      audio_output.setDevice(device);
    }
  }

  QAudioDevice MediaPlayer::findPreferredDevice() const {
    const auto devices = QMediaDevices::audioOutputs();
    for (const auto &device : std::as_const(devices)) {
      if (device.id() == output_device_id) {
        return device;
      }
    }
    return QAudioDevice();
  }

  void MediaPlayer::onAudioDevicesChanged() {
    // also fires when the system default changes — QMediaDevices has no
    // separate notifier for defaultAudioOutput
    devices_changed_debounce.start();
  }

  void MediaPlayer::evaluateAudioDevice() {
    ++device_change_epoch;
    const QAudioDevice preferred = findPreferredDevice(); // null in default mode too (empty id matches nothing)

    if (preferred.isNull()) {
      // default mode, or configured device unplugged: play through the current
      // default sink; the backend doesn't re-route by itself
      preferred_device_missing = !output_device_id.isEmpty();
      const QAudioDevice default_device = QMediaDevices::defaultAudioOutput();
      if (audio_output.device().id() != default_device.id()) {
        audio_output.setDevice(default_device);
        recoverPlayback(); // unwedge in case the pipeline stalled on a dead sink
      }
      return;
    }

    if (!preferred_device_missing && audio_output.device().id() == preferred.id()) {
      return; // unrelated hotplug: don't disturb playback
    }

    // configured device replugged: switch back to it. Double-set through default
    // with a delay because pipewire sometimes keeps the stream on the wrong sink.
    preferred_device_missing = false;
    audio_output.setDevice(QMediaDevices::defaultAudioOutput());
    const int epoch = device_change_epoch;
    QTimer::singleShot(100, this, [this, epoch]() {
      if (epoch != device_change_epoch) {
        return; // superseded by a newer device change
      }
      const QAudioDevice device = findPreferredDevice(); // re-resolve, may be gone again
      if (device.isNull()) {
        preferred_device_missing = true;
        audio_output.setDevice(QMediaDevices::defaultAudioOutput());
      } else {
        audio_output.setDevice(device);
      }
      recoverPlayback();
    });
  }

  void MediaPlayer::recoverPlayback() {
    if (state() != MediaPlayer::PlayingState) {
      return; // paused/stopped: play() runs unpause_workaround itself
    }
    unpause_workaround(); // seek nudge, usually enough to unwedge the backend
    // watchdog: if position is still frozen after the device switch, escalate to a
    // direct pause/play cycle. Not this->pause()/play(): those toggle and touch
    // next_after_stop/stream logic.
    const qint64 pos_before = player.position();
    const int epoch = device_change_epoch;
    QTimer::singleShot(500, this, [this, epoch, pos_before]() {
      if (epoch != device_change_epoch || state() != MediaPlayer::PlayingState) {
        return;
      }
      if (player.position() != pos_before) {
        return; // recovered
      }
      player.pause();
      player.play();
    });
  }
#endif
}
