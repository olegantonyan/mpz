#include "dispatch.h"
#include "rnjesus.h"

#include <QEventLoop>
#include <QTimer>

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
    if (current_playlist == nullptr || current_playlist->tracks().isEmpty()) {
      return;
    }

    bool is_random = (selected_playlist != nullptr && selected_playlist->random() == Playlist::Playlist::Random) ||
                     global_conf.playbackOrder() == "random";
    if (is_random) {
      int playlist_size = current_playlist->tracks().size();
      int rngjesus = RNJesus::generate(playlist_size);
      if (playlist_size > 1 && rngjesus == current_playlist->trackIndex(current_track_uid)) {
        // once again, but only once, you lucky bastard
        rngjesus = RNJesus::generate(playlist_size);
      }
      Track t = current_playlist->tracks().at(rngjesus);
      if (playlist_size > 1 && random_trail.exists(t.uid())) {
        rngjesus = RNJesus::generate(playlist_size);
        t = current_playlist->tracks().at(rngjesus);
      }

      emit play(t);
      random_trail.add(t.uid());
      return;
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto next = current + 1;
    if (next > current_playlist->tracks().size() - 1) {
      bool no_loop = (selected_playlist != nullptr && selected_playlist->random() == Playlist::Playlist::SequentialNoLoop) ||
                     global_conf.playbackOrder() == "sequential (no loop)";
      if (no_loop) {
        emit stop();
        return;
      }
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
    if (current_playlist == nullptr || current_playlist->tracks().isEmpty()) {
      return;
    }

    if (global_conf.playbackOrder() == "random") {
      auto prev_uid = random_trail.prev();
      if (prev_uid == current_track_uid) {
        prev_uid = random_trail.prev();
      }
      if (prev_uid != 0) {
        int prev_index = current_playlist->trackIndex(prev_uid);
        if (prev_index >= 0) {
          Track t = current_playlist->tracks().at(prev_index);
          emit play(t);
          return;
        }
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

  void Dispatch::on_trackChangedQuery(const QString &track_path, const QString &playlist_name_hint) {
    if (playlists->playlistsCount() == 0) {
      // on initial load there might no playlists yet
      QEventLoop loop;
      QTimer timer;
      auto conn_loop = connect(playlists, &PlaylistsUi::Controller::asyncLoadFinished, &loop, &QEventLoop::quit);
      auto conn_tmr = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
      timer.setSingleShot(true);
      timer.start(666);
      loop.exec();
      disconnect(conn_loop);
      disconnect(conn_tmr);
    }

    auto current_playlist = playlists->playlistByName(playlist_name_hint);
    if (!current_playlist) {
      current_playlist = playlists->currentPlaylist();
      if (!current_playlist) {
        return;
      }
    }
    if (current_playlist->tracks().isEmpty()) {
      // on initial app open current playlist may not be loaded yet
      // but mpd is playing
      // wait a bit for playlist load
      for (int i = 0; i < 15; i++) {
        QEventLoop loop;
        QTimer timer;
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.setSingleShot(true);
        timer.start(50);
        loop.exec();
        if (!current_playlist->tracks().isEmpty()) {
          break;
        }
      }
    }
    for (auto it : current_playlist->tracks()) {
      if (it.path() == track_path) {
        emit trackChangedQueryComplete(it);
        break;
      }
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
