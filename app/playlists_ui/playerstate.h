#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <QObject>

namespace PlaylistsUi {
  class PlayerState : public QObject {
  public:
    explicit PlayerState(QObject *parent = nullptr);

    void setSelectedPlaylist(quint64 playlist_uid);
    void setSelectedTrack(quint64 track_uid);

    void setPlaying(quint64 track_uid);

    quint64 playing_track() const;
    quint64 selected_track() const;
    quint64 selected_playlist() const;

    void resetPlaying();

  private:
    quint64 playing_track_uid;
    quint64 selected_track_uid;
    quint64 selected_playlist_uid;
  };
}
#endif // PLAYERSTATE_H
