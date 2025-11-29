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

  private:
    Track buildTrack(const MpdClient::Song &song, const QString &playlist_name);
    int extractYear(const QString& date);

    MpdClient::Client &client;
  };
}

#endif // MPDLOAER_H
