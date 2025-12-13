#ifndef DISPATCH_H
#define DISPATCH_H

#include "config/global.h"
#include "playlists_ui/playlistscontroller.h"
#include "playback/playerstate.h"
#include "playback/randomtrail.h"

#include <QObject>

namespace Playback {
  class Dispatch : public QObject {
    Q_OBJECT
  public:
    explicit Dispatch(Config::Global &conf, PlaylistsUi::Controller *playlists_ui);
    PlayerState &state();

  signals:
    void play(const Track &track);
    void stop();
    void trackChangedQueryComplete(const Track &track);

  public slots:
    void on_nextRequested();
    void on_prevRequested();
    void on_startRequested();
    void on_startFromPlaylistRequested(const std::shared_ptr<Playlist::Playlist> plst);
    void on_trackChangedQuery(const QString &track_path, const QString &playlist_name_hint);

  private:
    Config::Global &global_conf;
    PlayerState player_state;
    PlaylistsUi::Controller *playlists;
    RandomTrail random_trail;
  };
}

#endif // DISPATCH_H
