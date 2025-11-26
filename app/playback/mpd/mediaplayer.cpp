#include "mediaplayer.h"
#include "playlist/mpdloader.h"

namespace Playback {
  namespace Mpd {
  MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdClient::Client &cl, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, client(cl), playing_song_id(0) {
      connect(&client, &MpdClient::Client::playerStateChanged, this, &MediaPlayer::on_playerStateChanged);
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
      playing_song_id = 0;
    }

    void MediaPlayer::setPosition(qint64 position) {
    }

    void MediaPlayer::setVolume(int volume) {
      client.setVolume(volume);
    }

    void MediaPlayer::setTrack(const Track &track) {
      current_track = track;
      playing_song_id = 0;
    }

    void MediaPlayer::clearTrack() {
      current_track = Track();
      playing_song_id = 0;
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
        if (playing_song_id != 0 && this_song.id != playing_song_id) {
          emit trackChanged(this_song.filepath);
        }
        playing_song_id = this_song.id;
      }
      emit stateChanged(st);
      emit audioFormatUpdated(status.audioFormat.sampleRate, status.audioFormat.channels, status.audioFormat.bits);
    }
  }
}
