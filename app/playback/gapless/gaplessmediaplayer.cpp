#include "gaplessmediaplayer.h"

namespace Playback::Gapless {
  GaplessMediaPlayer::GaplessMediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, int cache_mb, QObject *parent) :
    MediaPlayer(stream_buffer_size, outdevid, parent),
    engine(qint64(cache_mb) * 1024 * 1024, stream_buffer_size) {
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
    return engine.state();
  }

  qint64 GaplessMediaPlayer::position() {
    return engine.positionMs();
  }

  void GaplessMediaPlayer::releaseAudio() {
    MediaPlayer::releaseAudio();
    engine.releaseAudio();
  }

  void GaplessMediaPlayer::pause() {
    if (track_set) {
      engine.pause();
    } else {
      MediaPlayer::pause();
    }
  }

  void GaplessMediaPlayer::play() {
    if (track_set) {
      engine.play();
    } else {
      MediaPlayer::play();
    }
  }

  void GaplessMediaPlayer::stop() {
    if (track_set) {
      engine.stop();
    } else {
      MediaPlayer::stop(); // the idle base synthesizes the StoppedState the engine won't re-emit
    }
  }

  void GaplessMediaPlayer::setPosition(qint64 position) {
    if (track_set) {
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

  void GaplessMediaPlayer::setReplayGainResolver(ReplayGainResolver fn) {
    engine.setReplayGainResolver(std::move(fn));
  }

  void GaplessMediaPlayer::refreshReplayGain() {
    engine.refreshReplayGain();
  }

  void GaplessMediaPlayer::setTrack(const Track &track) {
    track_set = true;
    engine.setTrack(track);
  }

  void GaplessMediaPlayer::prepareNextTrack(const Track &track) {
    if (!track_set) {
      return;
    }
    engine.prepareNextTrack(track.isStream() ? Track() : track);
  }

  void GaplessMediaPlayer::clearTrack() {
    if (track_set) {
      engine.clearTrack();
    } else {
      MediaPlayer::clearTrack();
    }
    track_set = false;
  }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  void GaplessMediaPlayer::setOutputDevice(QByteArray deviceid) {
    MediaPlayer::setOutputDevice(deviceid);
    engine.setOutputDevice(deviceid);
  }
#endif
}
