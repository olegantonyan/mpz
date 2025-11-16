#include "mpd_track.h"

namespace Playlist {
  Track MpdTrack::build(const struct mpd_song *song, const QString& playlist_name) {
    const char *uri = mpd_song_get_uri(song);
    const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    const char *genre = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
    const char *tracknum  = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
    const char *year   = mpd_song_get_tag(song, MPD_TAG_DATE, 0);

    Track track(
      QString::fromUtf8(uri),
      0,
      artist ? QString::fromUtf8(artist) : QString(),
      album ? QString::fromUtf8(album) : QString(),
      title ? QString::fromUtf8(title) : QString(),
      tracknum ? QString::fromUtf8(tracknum).toInt() : 0,
      year ? QString::fromUtf8(year).toInt() : 0,
      mpd_song_get_duration(song) * 1000,
      0,
      0,
      0
    );
    track.generateUidByHashing(playlist_name);
    return track;
  }
}
