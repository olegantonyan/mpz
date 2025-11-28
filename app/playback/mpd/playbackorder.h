#ifndef MPDPLAYBACKORER_H
#define MPDPLAYBACKORER_H

#include "track.h"
#include "playlist/playlist.h"
#include "playlists_ui/playlistscontroller.h"
#include "mpd_client/client.h"
#include "config/global.h"
#include "playback/dispatch.h"

#include <QObject>

namespace Playback {
  namespace Mpd {
    class PlaybackOrder : public QObject  {
      Q_OBJECT
    public:
      explicit PlaybackOrder(Config::Global &global_c, MpdClient::Client &cl, PlaylistsUi::Controller *playlists_ui, Playback::Dispatch *disptch, QObject *parent = nullptr);

    public slots:
      void updateByTrack(const Track &current_track);
      void updateByTrackUid(quint64 track_uid);
      void onOrderChanged();
      void onTrackSelected(const Track &track);

    signals:

    private slots:
      void onOptionsChanged();

    private:
      Config::Global &global_conf;
      MpdClient::Client &client;
      PlaylistsUi::Controller *playlists;
      Playback::Dispatch *dispatch;
    };
  }
}

#endif // MPDPLAYBACKORER_H
