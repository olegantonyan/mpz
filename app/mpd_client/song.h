#ifndef MPD_CLIENT_SONG_H
#define MPD_CLIENT_SONG_H

#include "mpd/client.h"

#include <QString>
#include <QMap>
#include <QStringList>
#include <QDebug>

namespace MpdClient {
  class Song {
    Q_GADGET
  public:
    Song() = default;
    explicit Song(const mpd_song *s);

    void updateFromMpdSong(const mpd_song *s);

    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString composer;
    QString performer;
    QString genre;
    QString date; // year
    QString name; // stream name
    int trackNumber = -1;
    int discNumber = -1;
    int duration = -1; // seconds
    int id = -1;
    int pos = -1;      // playlist position
    QString filepath;

    QString musicBrainzArtistId;
    QString musicBrainzAlbumId;
    QString musicBrainzAlbumArtistId;
    QString musicBrainzTrackId;

    QMap<int, QStringList> tags;
  };

  QDebug operator<<(QDebug dbg, const Song &s);
}

Q_DECLARE_METATYPE(MpdClient::Song)

#endif
