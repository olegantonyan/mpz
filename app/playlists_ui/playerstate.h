#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

#include <QObject>

namespace PlaylistsUi {
  class PlayerState : public QObject {
  public:
    explicit PlayerState(QObject *parent = nullptr);

    void setSelected(quint64 track_uid);
    void setPlaying(quint64 track_uid);

    quint64 playingTrack() const;
    quint64 selectedTrack() const;

    void resetPlaying();

  private:
    quint64 playing_track_uid;
    quint64 selected_track_uid;
  };
}
#endif // PLAYERSTATE_H
