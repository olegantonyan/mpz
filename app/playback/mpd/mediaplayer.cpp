#include "mediaplayer.h"
#include "playlist/mpdloader.h"

namespace Playback {
  namespace Mpd {
  MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdClient::Client &cl, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, client(cl) {
      connect(&client, &MpdClient::Client::playerStateChanged, this, &MediaPlayer::on_playerStateChanged);
      progress_timer.setInterval(250);
      connect(&progress_timer, &QTimer::timeout, this, &MediaPlayer::updateProgressNow);
    }

    MediaPlayer::State MediaPlayer::state() {
      return stateByStatus(client.status());
    }

    int MediaPlayer::volume() {
      int volume = qMax(client.status().volume, 0);
      return volume;
    }

    qint64 MediaPlayer::position() {
      return client.status().elapsedMs;
    }

    void MediaPlayer::pause() {
      if (state() == PausedState) {
        client.unpause();
      } else {
        client.pause();
      }
    }

    void MediaPlayer::play() {
      if (current_track.playlist_name().isEmpty()) {
        return;
      }

      auto playlist_tracks = Playlist::MpdLoader(client).playlistTracks(current_track.playlist_name());
      int pos = -1;
      for (int i = 0; i < playlist_tracks.size(); i++) {
        if (playlist_tracks.at(i).uid() == current_track.uid()) {
          pos = i;
          break;
        }
      }

      if (pos < 0) {
        qWarning() << QString("File not found in playlist: %1").arg(current_track.path());
        return;
      }
      client.play(current_track.playlist_name(), pos);
    }

    void MediaPlayer::stop() {
      client.stop();
    }

    void MediaPlayer::setPosition(qint64 position) {
      client.setPosition(position / 1000);
    }

    void MediaPlayer::setVolume(int volume) {
      client.setVolume(volume);
    }

    void MediaPlayer::setTrack(const Track &track) {
      current_track = track;
    }

    void MediaPlayer::clearTrack() {
      current_track = Track();
    }

    void MediaPlayer::next() {
      client.next();
    }

    void MediaPlayer::prev() {
      client.prev();
    }

    MediaPlayer::State MediaPlayer::stateByStatus(const MpdClient::Status &status) {
      State new_state = StoppedState;
      switch (status.state) {
      case MpdClient::Status::State::Play:
        new_state = PlayingState;
        break;
      case MpdClient::Status::State::Pause:
        new_state = PausedState;
        break;
      default:
        new_state = StoppedState;
        break;
      }
      return new_state;
    }

    void MediaPlayer::on_playerStateChanged() {
      auto status = client.status();
      auto st = stateByStatus(status);
      if (st == PlayingState) {
        auto this_song = client.currentSong();
        if (this_song.id != last_song.id && this_song.filepath != last_song.filepath) {
          emit trackChanged(this_song.filepath);
        }
        last_song = this_song;
        progress_timer.start();
        elapsed_clock.restart();
      }
      if (st == StoppedState || st == PausedState) {
        progress_timer.stop();
      }
      last_elapsed = status.elapsedMs;

      emit stateChanged(st);
      emit audioFormatUpdated(status.audioFormat.sampleRate, status.audioFormat.channels, status.audioFormat.bits);
    }

    void MediaPlayer::updateProgressNow() {
      int extra = elapsed_clock.elapsed();
      int pos = last_elapsed + extra;
      emit positionChanged(pos);
    }
  }
}
