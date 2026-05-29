#include "playback/native/mediaplayer.h"

#include <QMetaObject>
#include <QString>
#include <QUrl>

namespace Playback {
  namespace Native {

    MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent)
        : Playback::MediaPlayer(parent), output_device_id(outdevid) {
      Q_UNUSED(stream_buffer_size)

      // Engine callbacks fire from the decode thread; hop to the GUI thread
      // before touching Qt signals.
      engine.setErrorCallback([this](const std::string &m) {
        const QString msg = QString::fromStdString(m);
        QMetaObject::invokeMethod(this, [this, msg] { emit error(msg); }, Qt::QueuedConnection);
      });
      engine.setEndOfStreamCallback([this] {
        QMetaObject::invokeMethod(this, [this] { onEos(); }, Qt::QueuedConnection);
      });
      engine.setMetadataCallback([this](const std::map<std::string, std::string> &m) {
        StreamMetaData meta;
        for (const auto &kv : m) {
          meta.insert(QString::fromStdString(kv.first), QString::fromStdString(kv.second));
        }
        QMetaObject::invokeMethod(this, [this, meta] { emit streamMetaChanged(meta); }, Qt::QueuedConnection);
      });

      position_timer.setInterval(100);
      connect(&position_timer, &QTimer::timeout, this, &MediaPlayer::emitPosition);
      position_timer.start();

      // The engine opened the system-default device in its constructor; switch
      // to the persisted output device if one was configured and is present.
      if (!output_device_id.isEmpty()) {
        engine.setDeviceById(output_device_id.toStdString());
      }
    }

    MediaPlayer::~MediaPlayer() = default;

    Playback::MediaPlayer::State MediaPlayer::state() {
      switch (engine.state()) {
        case Engine::State::Playing: return PlayingState;
        case Engine::State::Paused:  return PausedState;
        case Engine::State::Stopped: return StoppedState;
      }
      return StoppedState;
    }

    int MediaPlayer::volume() { return volume_pct; }

    qint64 MediaPlayer::position() {
      const int64_t rel = engine.positionMs() - static_cast<int64_t>(offset_begin);
      return rel > 0 ? rel : 0;
    }

    void MediaPlayer::play() {
      if (synthetic_play) {
        synthetic_play = false;
        if (engine.state() == Engine::State::Paused) {
          engine.play();   // resume a paused same-file CUE track
        }
        // Either we just resumed, or we are already playing through a soft
        // boundary; synthesise the per-track PlayingState so the controller
        // emits started(track).
        emit stateChanged(PlayingState);
        return;
      }
      engine.play();
      emit stateChanged(PlayingState);
    }

    void MediaPlayer::pause() {
      if (engine.state() == Engine::State::Paused) {
        play();
      } else {
        engine.pause();
        emit stateChanged(PausedState);
      }
    }

    void MediaPlayer::stop() {
      synthetic_play = false;
      pending_boundary_next = false;
      engine.stop();
      emit stateChanged(StoppedState);
    }

    void MediaPlayer::setPosition(qint64 pos) {
      engine.seek(static_cast<int64_t>(offset_begin) + (pos < 0 ? 0 : pos));
    }

    void MediaPlayer::setVolume(int v) {
      volume_pct = v < 0 ? 0 : (v > 100 ? 100 : v);
      engine.setVolume(volume_pct / 100.0f);
    }

    void MediaPlayer::setTrack(const Track &track) {
      pending_boundary_next = false;

      if (track.isStream()) {
        current_source_path.clear();
        offset_begin = 0;
        offset_end = 0;
        synthetic_play = false;
        engine.open(track.url().toString().toStdString(), 0);
        return;
      }

      // Same-file CUE continuation: don't reopen. Sequential auto-advance keeps
      // playing gaplessly (the engine never stopped); a manual/random jump to a
      // region we are not currently inside does a cheap in-file seek.
      const bool same_file_cue =
          track.isCue() && !current_source_path.isEmpty() &&
          track.path() == current_source_path &&
          engine.state() != Engine::State::Stopped;

      if (same_file_cue) {
        offset_begin = track.begin();
        offset_end = offset_begin + track.duration();
        const int64_t pos = engine.positionMs();
        if (pos < static_cast<int64_t>(offset_begin) || pos >= static_cast<int64_t>(offset_end)) {
          engine.seek(static_cast<int64_t>(offset_begin));
        }
        synthetic_play = true;
        return;
      }

      offset_begin = track.isCue() ? track.begin() : 0;
      offset_end = track.isCue() ? offset_begin + track.duration() : 0;
      synthetic_play = false;
      engine.open(track.path().toStdString(), static_cast<int64_t>(offset_begin));
      current_source_path = track.path();
    }

    void MediaPlayer::clearTrack() {
      synthetic_play = false;
      pending_boundary_next = false;
      offset_begin = 0;
      offset_end = 0;
      current_source_path.clear();
      engine.stop();
    }

    QList<Playback::MediaPlayer::AudioDevice> MediaPlayer::audioOutputs() {
      QList<Playback::MediaPlayer::AudioDevice> out;
      const auto devices = engine.playbackDevices();
      out.reserve(static_cast<int>(devices.size()));
      for (const auto &dev : devices) {
        AudioDevice ad;
        ad.id = QByteArray::fromStdString(dev.id);
        ad.description = QString::fromStdString(dev.name);
        ad.is_default = dev.is_default;
        out.append(ad);
      }
      return out;
    }

    void MediaPlayer::setOutputDevice(QByteArray deviceid) {
      output_device_id = deviceid;
      engine.setDeviceById(deviceid.toStdString());
    }

    void MediaPlayer::onEos() {
      // Natural end of the source. If a CUE soft boundary already advanced the
      // track, the engine's trailing EOF is not a separate track change.
      if (pending_boundary_next) {
        pending_boundary_next = false;
        return;
      }
      emit nextRequested();
    }

    void MediaPlayer::emitPosition() {
      if (engine.state() != Engine::State::Playing) return;
      const int64_t abs = engine.positionMs();
      if (offset_end > 0 && abs >= static_cast<int64_t>(offset_end)) {
        // Soft CUE boundary: advance the window and ask for the next track while
        // the engine keeps playing the same file uninterrupted.
        offset_begin = offset_end;
        offset_end = 0;
        pending_boundary_next = true;
        emit positionChanged(0);
        emit nextRequested();
        return;
      }
      emit positionChanged(abs - static_cast<int64_t>(offset_begin));
    }

  } // namespace Native
} // namespace Playback
