#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <QObject>

namespace PlaylistsUi {
  class StateItem {
  public:
    StateItem() : track_uid(0), playlist_uid(0) {}

    quint64 track_uid;
    quint64 playlist_uid;
  };

  class PlayerState : public QObject {
  public:
    explicit PlayerState(QObject *parent = nullptr);

    void setSelectedPlaylist(quint64 uid);
    void setSelectedTrack(quint64 uid);

    void setPlayingPlaylist(quint64 uid);
    void setPlayingTrack(quint64 uid);

    StateItem selected() const;
    StateItem playing() const;

    void resetPlaying();

  private:
    StateItem _selected;
    StateItem _playing;
  };
}
#endif // PLAYERSTATE_H
