#include "coverart/online/albummatch.h"
#include "lyrics/textmatch.h"

namespace CoverArt {
  namespace Online {
    namespace {
      constexpr double ALBUM_WEIGHT = 0.55;
      constexpr double ARTIST_WEIGHT = 0.45;
      // Deliberately strict: a wrong cover is written to disk and outlives the
      // session, so showing nothing beats showing someone else's artwork.
      constexpr double ACCEPT_TOTAL = 0.72;
      constexpr double MIN_ALBUM_SCORE = 0.6;
      constexpr double MIN_ARTIST_SCORE = 0.45;
    }

    double AlbumMatch::score(const AlbumCandidate &candidate,
                             const QString &artist, const QString &album) {
      const QString want_album = Lyrics::TextMatch::normalizeTitle(album);
      const QString got_album = Lyrics::TextMatch::normalizeTitle(candidate.album);
      if (want_album.isEmpty() || got_album.isEmpty()) {
        return 0.0;
      }
      const double album_score = Lyrics::TextMatch::similarity(want_album, got_album);
      if (album_score < MIN_ALBUM_SCORE) {
        return 0.0;
      }

      const QString want_artist = Lyrics::TextMatch::normalizeArtist(artist);
      const QString got_artist = Lyrics::TextMatch::normalizeArtist(candidate.artist);
      double artist_score = Lyrics::TextMatch::similarity(want_artist, got_artist);
      // "A feat. B" on our side vs plain "A" upstream is a match, not a miss.
      const QString want_primary =
        Lyrics::TextMatch::normalizeArtist(Lyrics::TextMatch::primaryArtist(artist));
      if (want_primary != want_artist) {
        artist_score = qMax(artist_score, Lyrics::TextMatch::similarity(want_primary, got_artist));
      }
      if (artist_score < MIN_ARTIST_SCORE) {
        return 0.0;
      }

      return ALBUM_WEIGHT * album_score + ARTIST_WEIGHT * artist_score;
    }

    int AlbumMatch::bestCandidate(const QVector<AlbumCandidate> &candidates,
                                  const QString &artist, const QString &album) {
      int best_index = -1;
      double best_total = 0.0;
      for (int i = 0; i < candidates.size(); ++i) {
        const double total = score(candidates.at(i), artist, album);
        if (total >= ACCEPT_TOTAL && total > best_total) {
          best_total = total;
          best_index = i;
        }
      }
      return best_index;
    }
  }
}
