#ifndef DISPATCH_H
#define DISPATCH_H

#include "config/global.h"
#include "playlists_ui/playlistsview.h"
#include "playerstate.h"
#include "playback/playbackview.h"

#include <QObject>

namespace Playback {
  class Dispatch : public QObject {
    Q_OBJECT
  public:
    explicit Dispatch(Config::Global &conf, PlaylistsUi::View *playlists_ui);
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
    PlaylistsUi::View *playlists;
  };
}

#endif // DISPATCH_H
