#ifndef MPDPLAYBACKORER_H
#define MPDPLAYBACKORER_H

#include "track.h"
#include "playlist/playlist.h"
#include "playlists_ui/playlistscontroller.h"
#include "mpd_client/client.h"
#include "config/global.h"

#include <QObject>

namespace Playback {
  namespace Mpd {
    class PlaybackOrder : public QObject  {
      Q_OBJECT
    public:
      explicit PlaybackOrder(Config::Global &global_c, MpdClient::Client &cl, PlaylistsUi::Controller *playlists_ui, QObject *parent = nullptr);

    public slots:
      void update(const Track &current_track);
      void updateByTrackUid(quint64 track_uid);

    signals:

    private:
      Config::Global &global_conf;
      MpdClient::Client &client;
      PlaylistsUi::Controller *playlists;
    };
  }
}

#endif // MPDPLAYBACKORER_H
