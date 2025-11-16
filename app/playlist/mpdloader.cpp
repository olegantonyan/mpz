#include "mpdloader.h"

#include <QDebug>

namespace Playlist {
  MpdLoader::MpdLoader(const QDir &path, MpdConnection &conn) : Loader(path), connection(conn) {
    name = path.path();
  }

  QVector<Track> MpdLoader::tracks() const {
    QMutexLocker locker(&connection.mutex);

    QVector<Track> result;

    if (!mpd_send_list_playlist_meta(connection.conn, name.toUtf8().constData())) {
      qWarning() << "mpd_send_list_playlist_meta:" << connection.last_error();
      return result;
    }

    struct mpd_song *song;
    while ((song = mpd_recv_song(connection.conn)) != nullptr) {
      const char *uri = mpd_song_get_uri(song);
      const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
      const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
      const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
      const char *genre = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
      const char *tracknum  = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
      const char *year   = mpd_song_get_tag(song, MPD_TAG_DATE, 0);

      result << Track(
        uri ? QString::fromUtf8(uri) : QString(),
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

      mpd_song_free(song);
    }

    mpd_response_finish(connection.conn);

    return result;
  }

  bool MpdLoader::is_playlist_file() const {
    return false;
  }
}
