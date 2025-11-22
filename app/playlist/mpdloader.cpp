#include "mpdloader.h"

#include <QDebug>

namespace Playlist {
  MpdLoader::MpdLoader(MpdConnection &conn) : connection(conn) {
  }

  QVector<Track> MpdLoader::playlistTracks(const QString& playlist_name) {
    MpdConnectionLocker locker(connection);

    QVector<Track> result;

    if (!mpd_send_list_playlist_meta(connection.conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_send_list_playlist_meta:" << connection.lastError();
      return result;
    }

    struct mpd_song *song;
    while ((song = mpd_recv_song(connection.conn)) != nullptr) {
      result << buildTrack(song, playlist_name);
      mpd_song_free(song);
    }

    mpd_response_finish(connection.conn);

    return result;
  }

  QVector<Track> MpdLoader::dirsTracks(const QList<QDir> &filepaths, const QString &playlist_name) {
    MpdConnectionLocker locker(connection);

    QVector<Track> result;

    for (auto path : filepaths) {
      if (!mpd_send_list_all_meta(connection.conn, path.path().toUtf8().constData())) {
        qWarning() << "mpd_send_list_all_meta: " << connection.lastError();
        mpd_response_finish(connection.conn);
        return result;
      }

      struct mpd_song *song;
      while ((song = mpd_recv_song(connection.conn)) != nullptr) {
        result << buildTrack(song, playlist_name);
        mpd_song_free(song);
      }

      mpd_response_finish(connection.conn);
    }

    return result;
  }

  Track MpdLoader::buildTrack(const struct mpd_song *song, const QString& playlist_name) {
    const char *uri = mpd_song_get_uri(song);
    const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
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
    track.setPlaylistName(playlist_name);
    track.setMpd(true);
    return track;
  }
}
