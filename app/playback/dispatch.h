#ifndef DISPATCH_H
#define DISPATCH_H

#include "config/global.h"
#include "playlists_ui/playlistscontroller.h"
#include "playerstate.h"
#include "playback/playbackcontroller.h"

#include <QObject>

namespace Playback {
  class Dispatch : public QObject {
    Q_OBJECT
  public:
    explicit Dispatch(Config::Global &conf, PlaylistsUi::Controller *playlists_ui);
    PlayerState &state();

  signals:
    void play(const Track &track);

  public slots:
    void on_nextRequested();
    void on_prevRequested();
    void on_startRequested();

  private:
    Config::Global &global_conf;
    PlayerState player_state;
    PlaylistsUi::Controller *playlists;
  };
}

#endif // DISPATCH_H
