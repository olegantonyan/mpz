#include "dispatch.h"
#include "rnjesus.h"

namespace Playback {
  Dispatch::Dispatch(Config::Global &conf, PlaylistsUi::Controller *playlists_ui) :
    QObject(nullptr), global_conf(conf), playlists(playlists_ui) {
  }

  PlayerState &Dispatch::state() {
    return player_state;
  }

  void Dispatch::on_nextRequested() {
    auto selected_playlist = playlists->playlistByTrackUid(player_state.selectedTrack());
    if (global_conf.playbackFollowCursor()) {
      if (selected_playlist != nullptr) {
        auto selected_track = selected_playlist->trackBy(player_state.selectedTrack());
        if (!player_state.followedCursor() && player_state.playingTrack() != selected_track.uid()) {
          emit play(selected_track);
          return;
        }
      }
    }


    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }

    if (selected_playlist->random() == Playlist::Playlist::Random || global_conf.playbackOrder() == "random") {
      int rngjesus = RNJesus::generate(current_playlist->tracks().size() - 1);
      if (rngjesus == current_playlist->trackIndex(current_track_uid)) {
        // once again, but only once, you lucky bastard
        rngjesus = RNJesus::generate(current_playlist->tracks().size() - 1);
      }
      Track t = current_playlist->tracks().at(rngjesus);
      if (random_trail.exists(t.uid())) {
        rngjesus = RNJesus::generate(current_playlist->tracks().size() - 1);
        t = current_playlist->tracks().at(rngjesus);
      }

      emit play(t);
      random_trail.add(t.uid());
      return;
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto next = current + 1;
    if (next > current_playlist->tracks().size() - 1) {
      Track t = current_playlist->tracks().at(0);
      emit play(t);
    } else {
      Track t = current_playlist->tracks().at(next);
      emit play(t);
    }
  }

  void Dispatch::on_prevRequested() {
    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }

    if (global_conf.playbackOrder() == "random") {
      auto prev_uid = random_trail.prev();
      if (prev_uid == current_track_uid) {
        prev_uid = random_trail.prev();
      }
      if (prev_uid != 0) {
        Track t = current_playlist->tracks().at(current_playlist->trackIndex(prev_uid));
        emit play(t);
        return;
      }
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto prev = current - 1;
    if (prev < 0) {
      auto max = current_playlist->tracks().size() - 1;
      Track t = current_playlist->tracks().at(max);
      emit play(t);
    } else {
      Track t = current_playlist->tracks().at(prev);
      emit play(t);
    }
  }

  void Dispatch::on_startFromPlaylistRequested(const std::shared_ptr<Playlist::Playlist> plst) {
    if (plst != nullptr && !plst->tracks().isEmpty()) {
      Track t = plst->tracks().first();
      emit play(t);
    }
  }

  void Dispatch::on_startRequested() {
    quint64 selected_track_uid = player_state.selectedTrack();
    auto selected_playlist = playlists->playlistByTrackUid(selected_track_uid);
    if (selected_playlist != nullptr) {
      Track t = selected_playlist->trackBy(selected_track_uid);
      emit play(t);
    }
  }
}
