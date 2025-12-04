#include "mpdloader.h"
#include "playlist.h"

#include <QDebug>
#include <QDate>
#include <QRegularExpression>

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

  QVector<Track> MpdLoader::builTracksFromSongsSorted(const QVector<MpdClient::Song> &songs, const QString &playlist_name) {
    QVector<Track> tracks;
    for (auto it : songs) {
      tracks << buildTrack(it, playlist_name);
    }
    Playlist pl;
    pl.append(tracks, true);
    return pl.tracks();
  }

  Track MpdLoader::buildTrack(const MpdClient::Song &song, const QString& playlist_name) {
    Track track(
      song.filepath,
      0,
      song.artist.isEmpty() ? song.albumArtist : song.artist,
      song.album,
      song.title,
      song.trackNumber,
      extractYear(song.date),
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

  int MpdLoader::extractYear(const QString &date) {
    bool ok = false;
    int toint = date.toInt(&ok);
    if (ok) {
      return toint;
    }

    auto dt = QDate::fromString(date, Qt::ISODate);
    if (dt.isValid()) {
      return dt.year();
    }

    QRegularExpression re(R"((18|19|20)\d{2})");
    QRegularExpressionMatch match = re.match(date);
    if (match.hasMatch()) {
      return match.captured(0).toInt();
    }

    return 0;
  }
}
