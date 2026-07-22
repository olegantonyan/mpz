#include "gaplessmediaplayer.h"

namespace Playback::Gapless {
  GaplessMediaPlayer::GaplessMediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, int cache_mb, bool gapless_enabled, QObject *parent) :
    MediaPlayer(stream_buffer_size, outdevid, parent),
    engine(qint64(cache_mb) * 1024 * 1024, stream_buffer_size),
    gapless_enabled(gapless_enabled) {
    connect(&engine, &Engine::positionChanged, this, &MediaPlayer::positionChanged);
    connect(&engine, &Engine::stateChanged, this, &MediaPlayer::stateChanged);
    connect(&engine, &Engine::error, this, &MediaPlayer::error);
    connect(&engine, &Engine::nextRequested, this, &MediaPlayer::nextRequested);
    connect(&engine, &Engine::aboutToFinish, this, &MediaPlayer::aboutToFinish);
    connect(&engine, &Engine::streamBufferfillChanged, this, &MediaPlayer::streamBufferfillChanged);
    connect(&engine, &Engine::streamMetaChanged, this, &MediaPlayer::streamMetaChanged);
    connect(&engine, &Engine::effectiveOutputDeviceChanged, this, &GaplessMediaPlayer::effectiveOutputDeviceChanged);
    engine.setOutputDevice(outdevid);
  }

  MediaPlayer::State GaplessMediaPlayer::state() {
    return backend == Backend::Engine ? engine.state() : MediaPlayer::state();
  }

  qint64 GaplessMediaPlayer::position() {
    return backend == Backend::Engine ? engine.positionMs() : MediaPlayer::position();
  }

  void GaplessMediaPlayer::pause() {
    if (backend == Backend::Engine) {
      engine.pause();
    } else {
      MediaPlayer::pause();
    }
  }

  void GaplessMediaPlayer::play() {
    if (backend == Backend::Engine) {
      engine.play();
    } else {
      MediaPlayer::play();
    }
  }

  void GaplessMediaPlayer::stop() {
    if (backend == Backend::Engine) {
      engine.stop(); // never base stop() while Engine active: base synthesizes a spurious StoppedState from its idle QMediaPlayer
    } else {
      MediaPlayer::stop();
    }
  }

  void GaplessMediaPlayer::setPosition(qint64 position) {
    if (backend == Backend::Engine) {
      engine.setPositionMs(position);
    } else {
      MediaPlayer::setPosition(position);
    }
  }

  void GaplessMediaPlayer::setVolume(int volume) {
    MediaPlayer::setVolume(volume);
    engine.setVolume(volume);
  }

  void GaplessMediaPlayer::setEqualizer(const Eq::EqProfile &profile, bool enabled) {
    engine.setEqualizer(profile, enabled);
  }

  void GaplessMediaPlayer::setTrack(const Track &track) {
    if (!gapless_enabled) {
      if (backend == Backend::Engine) {
        engine.clearTrack();
      }
      backend = Backend::Qmp;
      MediaPlayer::setTrack(track);
    } else {
      if (backend == Backend::Qmp) {
        MediaPlayer::clearTrack();
      }
      backend = Backend::Engine;
      engine.setTrack(track);
    }
  }

  void GaplessMediaPlayer::prepareNextTrack(const Track &track) {
    if (backend != Backend::Engine) {
      return;
    }
    engine.prepareNextTrack(track.isStream() ? Track() : track);
  }

  void GaplessMediaPlayer::clearTrack() {
    if (backend == Backend::Engine) {
      engine.clearTrack();
    } else {
      MediaPlayer::clearTrack();
    }
    backend = Backend::None;
  }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  void GaplessMediaPlayer::setOutputDevice(QByteArray deviceid) {
    MediaPlayer::setOutputDevice(deviceid);
    engine.setOutputDevice(deviceid);
  }
#endif
}
