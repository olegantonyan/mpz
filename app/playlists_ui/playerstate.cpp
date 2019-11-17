#include "playerstate.h"

#include <QDebug>

namespace PlaylistsUi {
  PlayerState::PlayerState(QObject *parent) : QObject(parent) {
  }

  void PlayerState::setSelectedPlaylist(quint64 playlist_uid) {
    selected_playlist_uid = playlist_uid;
  }

  void PlayerState::setSelectedTrack(quint64 track_uid) {
    selected_track_uid = track_uid;
  }

  void PlayerState::setPlaying(quint64 track_uid) {
    playing_track_uid = track_uid;
  }

  quint64 PlayerState::playing_track() const {
    return playing_track_uid;
  }

  quint64 PlayerState::selected_track() const {
    return selected_track_uid;
  }

  quint64 PlayerState::selected_playlist() const {
    return selected_playlist_uid;
  }

  void PlayerState::resetPlaying() {
    playing_track_uid = 0;
  }
}
