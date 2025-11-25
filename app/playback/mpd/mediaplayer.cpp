#include "mediaplayer.h"
#include "playlist/mpdloader.h"

namespace Playback {
  namespace Mpd {
  MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdClient::Client &cl, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, client(cl), playing_song_id(0) {
      connect(&client, &MpdClient::Client::playerStateChanged, this, &MediaPlayer::on_playerStateChanged);
    }

    MediaPlayer::State MediaPlayer::state() {
      State new_state = StoppedState;
      switch (client.status().state) {
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

    int MediaPlayer::volume() {
      int volume = qMax(client.status().volume, 0);
      return volume;
    }

    qint64 MediaPlayer::position() {
      return 0;
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

      updateStatus();
    }

    void MediaPlayer::stop() {
      client.stop();
      playing_song_id = 0;
      updateStatus();
    }

    void MediaPlayer::setPosition(qint64 position) {
    }

    void MediaPlayer::setVolume(int volume) {
      //MpdConnectionLocker locker(connection);
      //if (!mpd_run_set_volume(connection.conn, volume)) {
      //   qWarning() << "mpd_run_set_volume:" << connection.lastError();
      //}
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
      updateStatus();
    }

    void MediaPlayer::prev() {
      client.prev();
      updateStatus();
    }

    void MediaPlayer::updateStatus() {
      //emit stateChanged(new_state);
    }

    QPair<unsigned int, QString> MediaPlayer::get_current_song() {
      QPair<unsigned int, QString> result;
      result.first = 0;
      result.second = "";

      /*auto *song = mpd_run_current_song(connection.conn);
      if (song) {
        result.first = mpd_song_get_id(song);
        result.second = QString::fromUtf8(mpd_song_get_uri(song));
        mpd_song_free(song);
      }*/
      return result;
    }

    void MediaPlayer::on_playerStateChanged() {
      qDebug() << "on_playerStateChanged";
      auto st = state();
      if (st == PlayingState) {
        auto this_song = get_current_song();
        if (playing_song_id != 0 && this_song.first != playing_song_id) {
          emit trackChanged(this_song.second);
        }
        playing_song_id = this_song.first;
      }
      emit stateChanged(st);
    }
  }
}
