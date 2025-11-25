#include "mpdloader.h"

#include <QDebug>

namespace Playlist {
MpdLoader::MpdLoader(MpdClient::Client &cl) : client(cl) {
  }

  QVector<Track> MpdLoader::playlistTracks(const QString& playlist_name) {
    QVector<Track> result;
    auto songs = client.lsPlaylistSongs(playlist_name);
    for (auto it : songs) {
      result << buildTrack(it, playlist_name);
    }
    return result;
  }

  QVector<Track> MpdLoader::dirsTracks(const QList<QDir> &filepaths, const QString &playlist_name) {
    QVector<Track> result;
    QStringList paths;
    for (auto it : filepaths) {
      paths << it.path();
    }
    auto songs = client.lsDirsSongs(paths);
    for (auto it : songs) {
      result << buildTrack(it, playlist_name);
    }
    return result;
  }

  Track MpdLoader::buildTrack(const MpdClient::Song &song, const QString& playlist_name) {
    Track track(
      song.filepath,
      0,
      song.artist,
      song.album,
      song.title,
      song.trackNumber,
      song.date.toInt(),
      song.duration * 1000,
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
