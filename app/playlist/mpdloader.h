#ifndef MPDLOAER_H
#define MPDLOAER_H

#include "track.h"
#include "mpdconnection.h"

#include <QString>
#include <QVector>
#include <QDir>

namespace Playlist {
  class MpdLoader {
  public:
    explicit MpdLoader(MpdConnection &conn);

    QVector<Track> playlistTracks(const QString& playlist_name);
    QVector<Track> dirsTracks(const QList<QDir> &filepaths, const QString &playlist_name = QString());

  private:
    Track buildTrack(const struct mpd_song *song, const QString& playlist_name);

    MpdConnection &connection;
  };
}

#endif // MPDLOAER_H
