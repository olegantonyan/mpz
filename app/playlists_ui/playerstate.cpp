#include "playerstate.h"

#include <QDebug>

namespace PlaylistsUi {
  PlayerState::PlayerState(QObject *parent) : QObject(parent) {
  }

  void PlayerState::setSelectedPlaylist(int idx) {
    _selected.playlist_index = idx;
  }

  void PlayerState::setSelectedTrack(int idx) {
    _selected.track_index = idx;
  }

  void PlayerState::setPlayingPlaylist(int idx) {
    _playing.playlist_index = idx;
  }

  void PlayerState::setPlayingTrack(int idx) {
    _playing.track_index = idx;
  }

  StateItem PlayerState::selected() const {
    return _selected;
  }

  StateItem PlayerState::playing() const {
    return  _playing;
  }

  void PlayerState::resetPlaying() {
    _playing.track_index = -1;
    _playing.playlist_index = -1;
  }
}
