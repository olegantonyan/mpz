#include "song.h"

namespace MpdClient {
  Song::Song(const mpd_song *s) {
    updateFromMpdSong(s);
  }

  void Song::updateFromMpdSong(const mpd_song *s) {
    if (!s) {
      return;
    }

    auto getTag = [s](mpd_tag_type tag) -> QString {
      const char *v = mpd_song_get_tag(s, tag, 0);
      return v ? QString(v) : QString();
    };

    title  = getTag(MPD_TAG_TITLE);
    artist = getTag(MPD_TAG_ARTIST);
    album  = getTag(MPD_TAG_ALBUM);
    albumArtist = getTag(MPD_TAG_ALBUM_ARTIST);
    composer = getTag(MPD_TAG_COMPOSER);
    performer = getTag(MPD_TAG_PERFORMER);
    genre = getTag(MPD_TAG_GENRE);
    date = getTag(MPD_TAG_DATE);
    name = getTag(MPD_TAG_NAME);

    trackNumber = getTag(MPD_TAG_TRACK).toInt();
    discNumber = getTag(MPD_TAG_DISC).toInt();

    duration = mpd_song_get_duration(s);
    id = mpd_song_get_id(s);
    pos = mpd_song_get_pos(s);
    filepath = mpd_song_get_uri(s);

    musicBrainzArtistId = getTag(MPD_TAG_MUSICBRAINZ_ARTISTID);
    musicBrainzAlbumId = getTag(MPD_TAG_MUSICBRAINZ_ALBUMID);
    musicBrainzAlbumArtistId = getTag(MPD_TAG_MUSICBRAINZ_ALBUMARTISTID);
    musicBrainzTrackId = getTag(MPD_TAG_MUSICBRAINZ_TRACKID);


    for (int tag = MPD_TAG_ARTIST; tag <= MPD_TAG_MUSICBRAINZ_TRACKID; ++tag) {
      const char *v = mpd_song_get_tag(s, static_cast<mpd_tag_type>(tag), 0);
      if (v) {
        QStringList list;
        list << QString(v);
        tags.insert(tag, list);
      }
    }
  }

  QDebug operator<<(QDebug dbg, const Song &s) {
    dbg.nospace() << "MpdClient::Song("
                  << "id=" << s.id
                  << ", pos=" << s.pos
                  << ", title=\"" << s.title << "\""
                  << ", artist=\"" << s.artist << "\""
                  << ", album=\"" << s.album << "\""
                  << ", albumArtist=\"" << s.albumArtist << "\""
                  << ", composer=\"" << s.composer << "\""
                  << ", performer=\"" << s.performer << "\""
                  << ", genre=\"" << s.genre << "\""
                  << ", date=\"" << s.date << "\""
                  << ", track=" << s.trackNumber
                  << ", disc=" << s.discNumber
                  << ", duration=" << s.duration << "s"
                  << ", filepath=\"" << s.filepath << "\""
                  << ", name=\"" << s.name << "\""
                  << ", MBArtistId=\"" << s.musicBrainzArtistId << "\""
                  << ", MBAlbumId=\"" << s.musicBrainzAlbumId << "\""
                  << ", MBAlbumArtistId=\"" << s.musicBrainzAlbumArtistId << "\""
                  << ", MBTrackId=\"" << s.musicBrainzTrackId << "\")";
    return dbg.space();
  }

}
