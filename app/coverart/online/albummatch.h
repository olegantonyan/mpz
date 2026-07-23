#ifndef COVERART_ONLINE_ALBUMMATCH_H
#define COVERART_ONLINE_ALBUMMATCH_H

#include <QString>
#include <QVector>

namespace CoverArt {
  namespace Online {
    struct AlbumCandidate {
      QString artist;
      QString album;
    };

    // Candidate scoring for online album search. Pure functions, no I/O.
    // Reuses Lyrics::TextMatch for normalization; the scoring itself is
    // separate because album lookups have no duration or sync signal to weigh.
    class AlbumMatch {
    public:
      // 0..1 for a candidate against the wanted artist/album.
      static double score(const AlbumCandidate &candidate,
                          const QString &artist, const QString &album);
      // Index of the best-scoring candidate above the acceptance threshold, or -1.
      static int bestCandidate(const QVector<AlbumCandidate> &candidates,
                               const QString &artist, const QString &album);
    };
  }
}

#endif // COVERART_ONLINE_ALBUMMATCH_H
