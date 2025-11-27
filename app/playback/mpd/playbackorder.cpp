#include "playbackorder.h"

namespace Playback {
  namespace Mpd {
    PlaybackOrder::PlaybackOrder(Config::Global &global_c, MpdClient::Client &cl, PlaylistsUi::Controller *playlists_ui, QObject *parent) : QObject{parent}, global_conf(global_c), client(cl), playlists(playlists_ui) {
    }

    void PlaybackOrder::update(const Track &current_track) {
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
  }
}
