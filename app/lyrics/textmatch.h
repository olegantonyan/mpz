#ifndef LYRICS_TEXTMATCH_H
#define LYRICS_TEXTMATCH_H

#include <QString>
#include <QVector>

namespace Lyrics {
  struct SongCandidate {
    QString title;
    QString artist;
    int duration_seconds = 0; // 0 = unknown
    bool has_synced = false;
    bool instrumental = false;
  };

  // Metadata normalization and candidate scoring for online lyrics search.
  // Pure functions, no I/O.
  class TextMatch {
  public:
    // Strips "(feat. X)", "(Remastered 2021)", "- Live" suffixes etc., folds
    // unicode punctuation, collapses whitespace. Falls back to the trimmed
    // input if stripping would leave nothing.
    static QString normalizeTitle(const QString &raw);
    // Same pipeline without title-decoration stripping.
    static QString normalizeArtist(const QString &raw);
    // First artist of "A feat. B" / "A & B" / "A; B" / "A, B".
    static QString primaryArtist(const QString &artist);
    // 0..1; max of Levenshtein ratio, token-reorder ratio and a subset bonus.
    static double similarity(const QString &a, const QString &b);
    // Index of the best-scoring candidate above the acceptance threshold, or -1.
    static int bestCandidate(const QVector<SongCandidate> &candidates,
                             const QString &artist, const QString &title,
                             int duration_seconds);
  };
}

#endif // LYRICS_TEXTMATCH_H
