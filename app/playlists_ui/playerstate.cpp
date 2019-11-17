#include "playerstate.h"

#include <QDebug>

namespace PlaylistsUi {
  PlayerState::PlayerState(QObject *parent) : QObject(parent) {
  }

  void PlayerState::setSelectedPlaylist(quint64 uid) {
    _selected.playlist_uid = uid;
  }

  void PlayerState::setSelectedTrack(quint64 uid) {
    _selected.track_uid = uid;
  }

  void PlayerState::setPlayingPlaylist(quint64 uid) {
    _playing.playlist_uid = uid;
  }

  void PlayerState::setPlayingTrack(quint64 uid) {
    _playing.track_uid = uid;
  }

  StateItem PlayerState::selected() const {
    return _selected;
  }

  StateItem PlayerState::playing() const {
    return  _playing;
  }

  void PlayerState::resetPlaying() {
    _playing.track_uid = 0;
    _playing.playlist_uid = 0;
  }
}
