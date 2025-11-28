#include "playbackorder.h"

namespace Playback {
  namespace Mpd {
  PlaybackOrder::PlaybackOrder(Config::Global &global_c, MpdClient::Client &cl, PlaylistsUi::Controller *playlists_ui, Dispatch *disptch, QObject *parent) : QObject{parent}, global_conf(global_c), client(cl), playlists(playlists_ui), dispatch(disptch) {
    }

    void PlaybackOrder::updateByTrack(const Track &current_track) {
      updateByTrackUid(current_track.uid());
    }

    void PlaybackOrder::updateByTrackUid(quint64 track_uid) {
      auto playlist = playlists->playlistByTrackUid(track_uid);

      if (!playlist) {
        return;
      }

      bool rng = (playlist->random() == Playlist::Playlist::Random) || (global_conf.playbackOrder() == "random");
      bool norepeat = (playlist->random() == Playlist::Playlist::SequentialNoLoop) || (global_conf.playbackOrder() == "sequential (no loop)");

      client.setRepeat(!norepeat);
      client.setRandom(rng);
    }

    void PlaybackOrder::onOrderChanged() {
      // mpd does not have separate per playlist setting
      // load these only when track starts
      /*quint64 uid = dispatch->state().playingTrack();
      if (uid > 0) {
        QMetaObject::invokeMethod(
          this,
          "updateByTrackUid",
          Qt::QueuedConnection,
          Q_ARG(quint64, uid)
        );
      }*/
    }
  }
}
