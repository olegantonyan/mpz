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
    pending_next.clear();
    random_trail.add(t.uid());
    emit play(t);
  }

  Track Dispatch::computeNextTrack(bool &ok, bool &should_stop) const {
    ok = false;
    should_stop = false;

    auto selected_playlist = playlists->playlistByTrackUid(player_state.selectedTrack());
    quint64 current_track_uid = player_state.playingTrack();
    auto current_playlist = playlists->playlistByTrackUid(current_track_uid);
    if (current_playlist == nullptr || current_playlist->tracks().isEmpty()) {
      return Track();
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
      ok = true;
      return current_playlist->tracks().at(chosen);
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto next = current + 1;
    if (next > current_playlist->tracks().size() - 1) {
      bool no_loop = (selected_playlist != nullptr && selected_playlist->random() == Playlist::Playlist::SequentialNoLoop) ||
                     global_conf.playbackOrder() == "sequential (no loop)";
      if (no_loop) {
        should_stop = true;
        return Track();
      }
      ok = true;
      return current_playlist->tracks().at(0);
    }
    ok = true;
    return current_playlist->tracks().at(next);
  }

  Track Dispatch::followCursorNext() const {
    if (!global_conf.playbackFollowCursor()) {
      return Track();
    }
    auto selected_playlist = playlists->playlistByTrackUid(player_state.selectedTrack());
    if (selected_playlist == nullptr) {
      return Track();
    }
    Track selected_track = selected_playlist->trackBy(player_state.selectedTrack());
    if (!player_state.followedCursor() && player_state.playingTrack() != selected_track.uid()) {
      return selected_track;
    }
    return Track();
  }

  void Dispatch::on_nextRequested() {
    Track follow = followCursorNext();
    if (follow.uid() != 0) {
      playTrack(follow);
      return;
    }

    if (pending_next.validFor(player_state.playingTrack())) {
      bool ok = false;
      bool should_stop = false;
      Track fresh = computeNextTrack(ok, should_stop);
      if (should_stop) {
        pending_next.clear(); // order changed to no-loop at the very end: end-of-track semantics win
        emit stop();
        return;
      }
      auto selected_playlist = playlists->playlistByTrackUid(player_state.selectedTrack());
      if (isRandomMode(selected_playlist) != pending_random_mode) {
        pending_next.clear(); // random-mode toggled in the final seconds: use the fresh pick
        if (ok) {
          playTrack(fresh);
        }
        return;
      }
      Track pending_track = pending_next.take();
      if (playlists->playlistByTrackUid(pending_track.uid()) != nullptr) {
        playTrack(pending_track);
        return;
      }
    }

    bool ok = false;
    bool should_stop = false;
    Track t = computeNextTrack(ok, should_stop);
    if (should_stop) {
      emit stop();
      return;
    }
    if (!ok) {
      return;
    }
    playTrack(t);
  }

  void Dispatch::on_aboutToFinish() {
    Track candidate = followCursorNext();
    bool ok = candidate.uid() != 0;
    bool should_stop = false;
    if (!ok) {
      candidate = computeNextTrack(ok, should_stop);
    }
    if (ok && !should_stop) {
      pending_next.set(player_state.playingTrack(), candidate);
      pending_random_mode = isRandomMode(playlists->playlistByTrackUid(player_state.selectedTrack()));
      emit prepareNext(candidate);
    } else {
      pending_next.clear();
      emit prepareNext(Track());
    }
  }

  void Dispatch::on_prevRequested() {
    pending_next.clear();
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
    for (const auto &it : current_playlist->tracks()) {
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
      playTrack(t);
    }
  }

  void Dispatch::on_started(const Track &t) {
    player_state.setPlaying(t.uid());
    player_state.setFollowedCursor();
  }

  void Dispatch::on_stopped() {
    pending_next.clear();
    player_state.resetPlaying();
  }

  void Dispatch::on_playlistContentChanged() {
    pending_next.clear();
    emit prepareNext(Track()); // clear the engine's prepared segment (removed track must not play at the boundary)
    const quint64 uid = player_state.playingTrack();
    if (uid == 0) {
      return;
    }
    if (playlists->playlistByTrackUid(uid) != nullptr) {
      return;
    }
    if (!global_conf.stopWhenTrackRemoved()) {
      return;
    }
    emit stop();
    emit unloadPlaylistView();
  }
}
