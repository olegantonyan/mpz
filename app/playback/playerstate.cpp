#include "playback/playerstate.h"

#include <QDebug>

namespace Playback {
  PlayerState::PlayerState() {
    resetPlaying();
    resetFolowedCursor();
  }

  void PlayerState::setSelected(quint64 track_uid) {
    selected_track_uid = track_uid;
  }

  void PlayerState::setPlaying(quint64 track_uid) {
    playing_track_uid = track_uid;
  }

  void PlayerState::setFollowedCursor() {
    followed_cursor = true;
  }

  quint64 PlayerState::playingTrack() const {
    return playing_track_uid;
  }

  quint64 PlayerState::selectedTrack() const {
    return selected_track_uid;
  }

  bool PlayerState::followedCursor() const {
    return followed_cursor;
  }

  void PlayerState::resetPlaying() {
    playing_track_uid = 0;
  }

  void PlayerState::resetFolowedCursor() {
    followed_cursor = false;
  }
}

