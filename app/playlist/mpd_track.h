#ifndef MPD_TACK_H
#define MPD_TACK_H

#include "track.h"
#include "mpdconnection.h"

#include <QString>

namespace Playlist {
  class MpdTrack {
  public:
    static Track build(const struct mpd_song *song, const QString& playlist_name = QString());
  };
}

#endif // MPD_TACK_H
