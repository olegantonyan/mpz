#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <QObject>

namespace PlaylistsUi {
  class StateItem {
  public:
    StateItem() : track_index(-1), playlist_index(-1) {}

    int track_index;
    int playlist_index;
  };

  class PlayerState : public QObject {
  public:
    explicit PlayerState(QObject *parent = nullptr);

    void setSelectedPlaylist(int idx);
    void setSelectedTrack(int idx);

    void setPlayingPlaylist(int idx);
    void setPlayingTrack(int idx);

    StateItem selected() const;
    StateItem playing() const;

    void resetPlaying();

  private:
    StateItem _selected;
    StateItem _playing;
  };
}
#endif // PLAYERSTATE_H
