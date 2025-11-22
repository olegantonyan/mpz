#include "mediaplayer.h"
#include "playlist/mpdloader.h"

namespace Playback {
  namespace Mpd {
    MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdConnection &conn, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, connection(conn) {
      connect(&connection, &MpdConnection::connected, this, &MediaPlayer::on_connected);
      connect(&connection, &MpdConnection::playerStateChanged, this, &MediaPlayer::on_playerStateChanged);
    }

    MediaPlayer::State Playback::Mpd::MediaPlayer::state() {
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

    int Playback::Mpd::MediaPlayer::volume() const {
      return 0;
    }

    qint64 Playback::Mpd::MediaPlayer::position() const {
      return 0;
    }

    void Playback::Mpd::MediaPlayer::pause() {
      MpdConnectionLocker locker(connection);

      if (!mpd_run_pause(connection.conn, state() == PausedState ? false : true)) {
        qWarning() << "mpd_run_pause:" << connection.lastError();
      }
      return;
    }

    void Playback::Mpd::MediaPlayer::play() {
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

    void Playback::Mpd::MediaPlayer::stop() {
      MpdConnectionLocker locker(connection);
      if (!mpd_run_stop(connection.conn)) {
        return;
      }
      updateStatus();
    }

    void Playback::Mpd::MediaPlayer::setPosition(qint64 position) {
    }

    void Playback::Mpd::MediaPlayer::setVolume(int volume) {
    }

    void Playback::Mpd::MediaPlayer::setTrack(const Track &track) {
      current_track = track;
    }

    void Playback::Mpd::MediaPlayer::clearTrack() {
      current_track = Track();
    }

    void Playback::Mpd::MediaPlayer::setOutputDevice(QByteArray deviceid) {
    }

    void MediaPlayer::updateStatus() {


      //emit stateChanged(new_state);
    }

    void MediaPlayer::on_connected(const QUrl &url) {
    }

    void MediaPlayer::on_playerStateChanged() {
      emit stateChanged(state());
    }
  }
}
