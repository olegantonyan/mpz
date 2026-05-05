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

  bool Dispatch::isRandomMode(const std::shared_ptr<Playlist::Playlist> &plst) const {
    if (plst != nullptr && plst->random() == Playlist::Playlist::Random) {
      return true;
    }
    return global_conf.playbackOrder() == "random";
  }

  void Dispatch::playTrack(const Track &t) {
    random_trail.add(t.uid());
    emit play(t);
  }

  void Dispatch::on_nextRequested() {
    auto selected_playlist = playlists->playlistByTrackUid(player_state.selectedTrack());
    if (global_conf.playbackFollowCursor()) {
      if (selected_playlist != nullptr) {
        auto selected_track = selected_playlist->trackBy(player_state.selectedTrack());
        if (!player_state.followedCursor() && player_state.playingTrack() != selected_track.uid()) {
          playTrack(selected_track);
          return;
        }
      }
    }


    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr || current_playlist->tracks().isEmpty()) {
      return;
    }

    if (isRandomMode(selected_playlist)) {
      int playlist_size = current_playlist->tracks().size();
      int current_idx = current_playlist->trackIndex(current_track_uid);
      int avoid_window = qMin(playlist_size - 1, 16);
      int chosen = -1;
      const int attempts = qMin(playlist_size * 4, 64);
      for (int i = 0; i < attempts; i++) {
        int candidate = RNJesus::generate(playlist_size);
        if (playlist_size > 1 && candidate == current_idx) {
          continue;
        }
        Track t = current_playlist->tracks().at(candidate);
        if (random_trail.recentlyPlayed(t.uid(), avoid_window)) {
          continue;
        }
        chosen = candidate;
        break;
      }
      if (chosen < 0) {
        chosen = RNJesus::generate(playlist_size);
        if (playlist_size > 1 && chosen == current_idx) {
          chosen = (chosen + 1) % playlist_size;
        }
      }
      Track t = current_playlist->tracks().at(chosen);
      playTrack(t);
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
      playTrack(t);
    } else {
      Track t = current_playlist->tracks().at(next);
      playTrack(t);
    }
  }

  void Dispatch::on_prevRequested() {
    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr || current_playlist->tracks().isEmpty()) {
      return;
    }

    auto selected_playlist = playlists->playlistByTrackUid(player_state.selectedTrack());
    if (isRandomMode(selected_playlist)) {
      auto prev_uid = random_trail.goPrev();
      if (prev_uid != 0) {
        auto prev_playlist = playlists->playlistByTrackUid(prev_uid);
        if (prev_playlist == nullptr) {
          prev_playlist = current_playlist;
        }
        int prev_index = prev_playlist->trackIndex(prev_uid);
        if (prev_index >= 0) {
          Track t = prev_playlist->tracks().at(prev_index);
          // Navigate without re-adding: goPrev already moved the cursor.
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
      playTrack(t);
    } else {
      Track t = current_playlist->tracks().at(prev);
      playTrack(t);
    }
  }

  void Dispatch::on_startFromPlaylistRequested(const std::shared_ptr<Playlist::Playlist> plst) {
    if (plst != nullptr && !plst->tracks().isEmpty()) {
      Track t = plst->tracks().first();
      playTrack(t);
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
        random_trail.add(it.uid());
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
      playTrack(t);
    }
  }
}
