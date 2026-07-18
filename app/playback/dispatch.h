#ifndef DISPATCH_H
#define DISPATCH_H

#include "config/global.h"
#include "playlists_ui/playlistscontroller.h"
#include "playback/playerstate.h"
#include "playback/randomtrail.h"
#include "playback/pendingnext.h"

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
    void unloadPlaylistView();
    void trackChangedQueryComplete(const Track &track);
    void prepareNext(const Track &track);

  public slots:
    void on_nextRequested();
    void on_aboutToFinish();
    void on_prevRequested();
    void on_startRequested();
    void on_startFromPlaylistRequested(const std::shared_ptr<Playlist::Playlist> plst);
    void on_trackChangedQuery(const QString &track_path, const QString &playlist_name_hint);
    void on_started(const Track &t);
    void on_stopped();
    void on_playlistContentChanged();

  private:
    bool isRandomMode(const std::shared_ptr<Playlist::Playlist> &plst) const;
    void playTrack(const Track &t);
    Track computeNextTrack(bool &ok, bool &should_stop) const;
    Track followCursorNext() const;

    Config::Global &global_conf;
    PlayerState player_state;
    PlaylistsUi::Controller *playlists;
    RandomTrail random_trail;
    PendingNext pending_next;
    bool pending_random_mode = false; // random-mode flag captured when the pending pick was pre-committed
  };
}

#endif // DISPATCH_H
