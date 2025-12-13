#ifndef MPDLOAER_H
#define MPDLOAER_H

#include "track.h"
#include "mpd_client/client.h"

#include <QString>
#include <QVector>
#include <QDir>

namespace Playlist {
  class MpdLoader {
  public:
    explicit MpdLoader(MpdClient::Client &cl);

    QVector<Track> playlistTracks(const QString& playlist_name);
    QVector<Track> dirsTracks(const QList<QDir> &filepaths, const QString &playlist_name = QString());
    QVector<Track> builTracksFromSongsSorted(const QVector<MpdClient::Song> &songs, const QString &playlist_name);

  private:
    int extractYear(const QString& date);
    Track buildTrack(const MpdClient::Song &song, const QString &playlist_name);

    MpdClient::Client &client;
  };
}

#endif // MPDLOAER_H
