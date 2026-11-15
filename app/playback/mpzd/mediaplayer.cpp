#include "mediaplayer.h"
#include "streammetadata.h"

#include <QRegularExpression>

namespace Playback {
  namespace Mpzd {
    constexpr int PROGRESS_MS = 500;
    constexpr int RESYNC_MS = 5000;

    MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpzdClient::Client &cl, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, client(cl) {
      progress_timer.setInterval(PROGRESS_MS);
      resync_timer.setInterval(RESYNC_MS);
      connect(&progress_timer, &QTimer::timeout, this, &MediaPlayer::updateProgressNow);
      connect(&resync_timer, &QTimer::timeout, &client, &MpzdClient::Client::refreshStatus);
      connect(&client, &MpzdClient::Client::statusChanged, this, &MediaPlayer::onStatus);
      connect(&client, &MpzdClient::Client::disconnected, this, &MediaPlayer::onDisconnected);
    }

    MediaPlayer::State MediaPlayer::state() {
      return current_state;
    }

    int MediaPlayer::volume() {
      return last_volume < 0 ? client.status().volume : last_volume;
    }

    qint64 MediaPlayer::position() {
      if (current_state == PlayingState && elapsed_clock.isValid()) {
        return base_pos_ms + elapsed_clock.elapsed();
      }
      return base_pos_ms;
    }

    void MediaPlayer::pause() {
      if (current_state == PausedState) {
        client.request({"play"});
      } else if (current_state == PlayingState) {
        client.request({"pause"});
      }
    }

    void MediaPlayer::play() {
      if (track_changed && current_track.uid() != 0) {
        track_changed = false;
        client.request({"play", QString::number(MpzdClient::playlistIdOf(current_track.uid())), QString::number(MpzdClient::trackIdOf(current_track.uid()))});
        return;
      }
      client.request({"play"});
    }

    void MediaPlayer::stop() {
      client.request({"stop"});
    }

    void MediaPlayer::setPosition(qint64 position) {
      client.request({"seek", QString::number(position / 1000.0, 'f', 3)});
    }

    void MediaPlayer::setVolume(int volume) {
      last_volume = volume;
      client.request({"volume", QString::number(volume)});
    }

    void MediaPlayer::setTrack(const Track &track) {
      current_track = track;
      track_changed = true;
    }

    void MediaPlayer::clearTrack() {
      current_track = Track();
      track_changed = false;
    }

    void MediaPlayer::next() {
      client.request({"next"});
    }

    void MediaPlayer::prev() {
      client.request({"prev"});
    }

    MediaPlayer::State MediaPlayer::stateOf(const MpzdClient::Status &status) {
      switch (status.state) {
      case MpzdClient::Status::Play:
        return PlayingState;
      case MpzdClient::Status::Pause:
        return PausedState;
      case MpzdClient::Status::Stop:
      default:
        return StoppedState;
      }
    }

    void MediaPlayer::onStatus(const MpzdClient::Status &status) {
      const State st = stateOf(status);
      const quint64 uid = st != StoppedState && status.track != 0 ? MpzdClient::trackUid(status.playlist, status.track) : 0;
      const bool switched = uid != 0 && uid != current_uid;

      base_pos_ms = status.pos_ms;
      elapsed_clock.restart();

      if (status.volume != last_volume) {
        last_volume = status.volume;
        emit volumeReported(status.volume);
      }

      if (switched) {
        Track t = MpzdClient::trackFromRecord(status.track_record, status.playlist, QString(), client.currentUrl());
        static const QRegularExpression rate_re("(\\d+)Hz");
        static const QRegularExpression chans_re("(\\d+)ch");
        const auto rate = rate_re.match(status.format);
        const auto chans = chans_re.match(status.format);
        if (rate.hasMatch() || chans.hasMatch()) {
          t.setAudioFormat(rate.captured(1).toUInt(), static_cast<quint8>(chans.captured(1).toUInt()), 0);
        }
        current_track = t;
        track_changed = false;
        last_icy.clear();
        emit trackLoaded(t);
      }
      current_uid = uid;

      if (st != current_state || (switched && st != StoppedState)) {
        current_state = st;
        emit stateChanged(st);
      }

      if (uid != 0 && current_track.isStream() && status.icy != last_icy) {
        last_icy = status.icy;
        emit streamMetaChanged(StreamMetaData(status.icy));
      }

      if (st == PlayingState) {
        progress_timer.start();
        resync_timer.start();
      } else {
        progress_timer.stop();
        resync_timer.stop();
      }
      if (st != StoppedState) {
        updateProgressNow();
      }
    }

    void MediaPlayer::onDisconnected() {
      progress_timer.stop();
      resync_timer.stop();
      current_uid = 0;
      last_volume = -1;
      if (current_state != StoppedState) {
        current_state = StoppedState;
        emit stateChanged(StoppedState);
      }
    }

    void MediaPlayer::updateProgressNow() {
      emit positionChanged(position());
    }
  }
}
