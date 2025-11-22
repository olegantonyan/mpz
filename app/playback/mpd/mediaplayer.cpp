#include "mediaplayer.h"
#include "playlist/mpdloader.h"

namespace Playback {
  namespace Mpd {
    MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdConnection &conn, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, connection(conn), playing_song_id(0) {
      connect(&connection, &MpdConnection::connected, this, &MediaPlayer::on_connected);
      connect(&connection, &MpdConnection::playerStateChanged, this, &MediaPlayer::on_playerStateChanged);
    }

    MediaPlayer::State MediaPlayer::state() {
      MpdConnectionLocker locker(connection);
      State new_state = StoppedState;

      mpd_status *status = mpd_run_status(connection.conn);
      if (!status) {
        qWarning() << "mediaplyer mpd_run_status: " << current_track.path();
        return new_state;
      }

      enum mpd_state st = mpd_status_get_state(status);
      switch (st) {
      case MPD_STATE_PLAY:
        new_state = PlayingState;
        break;
      case MPD_STATE_PAUSE:
        new_state = PausedState;
        break;
      default:
        new_state = StoppedState;
        break;
      }
      return new_state;
    }

    int MediaPlayer::volume() const {
      return 0;
    }

    qint64 MediaPlayer::position() const {
      return 0;
    }

    void MediaPlayer::pause() {
      MpdConnectionLocker locker(connection);

      if (!mpd_run_pause(connection.conn, state() == PausedState ? false : true)) {
        qWarning() << "mpd_run_pause:" << connection.lastError();
      }
      return;
    }

    void MediaPlayer::play() {
      if (current_track.playlist_name().isEmpty()) {
        return;
      }

      MpdConnectionLocker locker(connection);

      if (!mpd_run_clear(connection.conn)) {
        qWarning() << "mpd_run_clear:" << connection.lastError();
        return;
      }
      if (!mpd_run_load(connection.conn, current_track.playlist_name().toUtf8().constData())) {
        qWarning() << "mpd_run_load:" << connection.lastError();
        return;
      }

      if (!mpd_run_idle_mask(connection.conn, MPD_IDLE_QUEUE)) {
        qWarning() << "mpd_run_idle_mask:" << connection.lastError();
        return;
      }

      auto playlist_tracks = Playlist::MpdLoader(connection).playlistTracks(current_track.playlist_name());
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

      if (!mpd_run_play_pos(connection.conn, pos)) {
        return;
      }

      updateStatus();
    }

    void MediaPlayer::stop() {
      MpdConnectionLocker locker(connection);
      if (!mpd_run_stop(connection.conn)) {
        return;
      }
      playing_song_id = 0;
      updateStatus();
    }

    void MediaPlayer::setPosition(qint64 position) {
    }

    void MediaPlayer::setVolume(int volume) {
    }

    void MediaPlayer::setTrack(const Track &track) {
      current_track = track;
    }

    void MediaPlayer::clearTrack() {
      current_track = Track();
    }

    void MediaPlayer::setOutputDevice(QByteArray deviceid) {
    }

    void MediaPlayer::next() {
      MpdConnectionLocker locker(connection);
      if (!mpd_run_next(connection.conn)) {
        return;
      }
      updateStatus();
    }

    void MediaPlayer::prev() {
      MpdConnectionLocker locker(connection);
      if (!mpd_run_previous(connection.conn)) {
        return;
      }
      updateStatus();
    }

    void MediaPlayer::updateStatus() {


      //emit stateChanged(new_state);
    }

    QPair<unsigned int, QString> MediaPlayer::get_current_song() {
      MpdConnectionLocker locker(connection);
      QPair<unsigned int, QString> result;
      result.first = 0;
      result.second = "";

      auto *song = mpd_run_current_song(connection.conn);
      if (song) {
        result.first = mpd_song_get_id(song);
        result.second = QString::fromUtf8(mpd_song_get_uri(song));
        mpd_song_free(song);
      }
      return result;
    }

    void MediaPlayer::on_connected(const QUrl &url) {
    }

    void MediaPlayer::on_playerStateChanged() {
      auto st = state();
      if (st == PlayingState) {
        auto this_song = get_current_song();
        if (this_song.first != playing_song_id) {
          qDebug() << this_song.second;
          emit trackChanged();
        }
        playing_song_id = this_song.first;
      }
      emit stateChanged(st);
    }
  }
}
