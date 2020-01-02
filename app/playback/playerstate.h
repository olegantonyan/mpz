#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <QtGlobal>

namespace Playback {
  class PlayerState {
  public:
    explicit PlayerState();

    void setSelected(quint64 track_uid);
    void setPlaying(quint64 track_uid);
    void setFollowedCursor();

    quint64 playingTrack() const;
    quint64 selectedTrack() const;
    bool followedCursor() const;

    void resetPlaying();
    void resetFolowedCursor();

  private:
    quint64 playing_track_uid;
    quint64 selected_track_uid;
    bool followed_cursor;
  };
}

#endif // PLAYERSTATE_H
