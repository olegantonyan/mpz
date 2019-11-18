#include "playerstate.h"

#include <QDebug>

PlayerState::PlayerState(QObject *parent) : QObject(parent) {
}

void PlayerState::setSelected(quint64 track_uid) {
  selected_track_uid = track_uid;
}

void PlayerState::setPlaying(quint64 track_uid) {
  playing_track_uid = track_uid;
}

quint64 PlayerState::playingTrack() const {
  return playing_track_uid;
}

quint64 PlayerState::selectedTrack() const {
  return selected_track_uid;
}

void PlayerState::resetPlaying() {
  playing_track_uid = 0;
}

