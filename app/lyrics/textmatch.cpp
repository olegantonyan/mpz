#include "lyrics/textmatch.h"

#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <algorithm>

namespace {
  // Scoring weights and acceptance thresholds (tune here).
  constexpr double kTitleWeight = 0.45;
  constexpr double kArtistWeight = 0.35;
  constexpr double kDurationWeight = 0.20;
  constexpr double kSyncedTieBreak = 0.01;
  constexpr double kAcceptTotal = 0.72;
  constexpr double kMinTitleScore = 0.6;
  constexpr double kMinArtistScore = 0.4;
  constexpr double kNeutralScore = 0.5; // when a field is unknown
  constexpr double kSubsetFloor = 0.85; // tokens of one are a subset of the other
  constexpr int kDurationExactSlack = 2;  // seconds
  constexpr int kDurationMaxSlack = 10;   // seconds

  // Title decorations commonly appended by tags/databases.
  const char *kDecorationKeywords =
    "remaster(?:ed)?|re-?recorded|live|mono|stereo|acoustic|demo|deluxe|"
    "edit(?:ed)?|version|mix|remix|single|bonus|anniversary|expanded|"
    "explicit|clean|radio";

  QString foldPunctuation(QString s) {
    static const QRegularExpression apostrophe_re(QStringLiteral("[‘’ʼ`´]"));
    static const QRegularExpression quote_re(QStringLiteral("[“”„]"));
    static const QRegularExpression dash_re(QStringLiteral("[‐‑‒–—―−]"));
    s.replace(apostrophe_re, QStringLiteral("'"));
    s.replace(quote_re, QStringLiteral("\""));
    s.replace(dash_re, QStringLiteral("-"));
    s.replace(QChar(0x2026), QStringLiteral("..."));
    return s;
  }

  QString stripFeat(QString s) {
    static const QRegularExpression bracketed_re(
      QStringLiteral("\\s*[(\\[]\\s*(?:feat\\.?|ft\\.?|featuring|with)\\s[^)\\]]*[)\\]]"),
      QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression bare_re(
      QStringLiteral("\\s+(?:feat\\.?|ft\\.?|featuring)\\s+.+$"),
      QRegularExpression::CaseInsensitiveOption);
    s.remove(bracketed_re);
    s.remove(bare_re);
    return s;
  }

  QString stripTitleDecorations(QString s) {
    static const QRegularExpression paren_re(
      QStringLiteral("\\s*[(\\[](?=[^)\\]]*\\b(?:%1)\\b)[^)\\]]*[)\\]]").arg(kDecorationKeywords),
      QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression dash_suffix_re(
      QStringLiteral("\\s+-\\s+[^-]*\\b(?:%1)\\b[^-]*$").arg(kDecorationKeywords),
      QRegularExpression::CaseInsensitiveOption);
    s.remove(paren_re);
    // Loop to handle stacked suffixes: "X - Live - 2011 Remaster".
    while (dash_suffix_re.match(s).hasMatch()) {
      s.remove(dash_suffix_re);
    }
    return s;
  }

  QString collapseWhitespace(QString s) {
    static const QRegularExpression ws_re(QStringLiteral("\\s+"));
    return s.replace(ws_re, QStringLiteral(" ")).trimmed();
  }

  QString normalize(const QString &raw, bool title) {
    QString s = raw.normalized(QString::NormalizationForm_KC);
    s = foldPunctuation(s);
    s = stripFeat(s);
    if (title) {
      s = stripTitleDecorations(s);
    }
    s = collapseWhitespace(s);
    if (s.isEmpty()) { // over-stripping guard, e.g. a track literally titled "Live"
      return collapseWhitespace(raw);
    }
    return s;
  }

  // Casefolded, punctuation-free form used for similarity comparison only.
  QString canonical(const QString &s) {
    static const QRegularExpression nonword_re(QStringLiteral("[^\\p{L}\\p{N}\\s]"));
    QString c = s.normalized(QString::NormalizationForm_KC).toCaseFolded();
    c.remove(nonword_re);
    return collapseWhitespace(c);
  }

  int levenshtein(const QString &a, const QString &b) {
    const int n = a.size();
    const int m = b.size();
    QVector<int> prev(m + 1), curr(m + 1);
    for (int j = 0; j <= m; ++j) {
      prev[j] = j;
    }
    for (int i = 1; i <= n; ++i) {
      curr[0] = i;
      for (int j = 1; j <= m; ++j) {
        const int cost = a[i - 1] == b[j - 1] ? 0 : 1;
        curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
      }
      prev.swap(curr);
    }
    return prev[m];
  }

  double levenshteinRatio(const QString &a, const QString &b) {
    const int longest = std::max(a.size(), b.size());
    if (longest == 0) {
      return 1.0;
    }
    return 1.0 - static_cast<double>(levenshtein(a, b)) / longest;
  }
}

namespace Lyrics {
  QString TextMatch::normalizeTitle(const QString &raw) {
    return normalize(raw, true);
  }

  QString TextMatch::normalizeArtist(const QString &raw) {
    return normalize(raw, false);
  }

  QString TextMatch::primaryArtist(const QString &artist) {
    static const QRegularExpression split_re(
      QStringLiteral("\\s*(?:;|/|,|&|\\bfeat\\.?\\s|\\bft\\.?\\s|\\bfeaturing\\b|\\bvs\\.?\\b)\\s*"),
      QRegularExpression::CaseInsensitiveOption);
    const auto parts = artist.split(split_re);
    for (const auto &part : parts) {
      const QString trimmed = part.trimmed();
      if (!trimmed.isEmpty()) {
        return trimmed;
      }
    }
    return artist.trimmed();
  }

  double TextMatch::similarity(const QString &a, const QString &b) {
    const QString ca = canonical(a);
    const QString cb = canonical(b);
    if (ca.isEmpty() && cb.isEmpty()) {
      return 1.0;
    }
    if (ca.isEmpty() || cb.isEmpty()) {
      return 0.0;
    }
    double result = levenshteinRatio(ca, cb);

    // canonical() collapses whitespace, so splitting on ' ' yields no empties.
    QStringList ta = ca.split(' ');
    QStringList tb = cb.split(' ');
    std::sort(ta.begin(), ta.end());
    std::sort(tb.begin(), tb.end());
    result = std::max(result, levenshteinRatio(ta.join(' '), tb.join(' ')));

    QSet<QString> sa, sb;
    for (const auto &t : ta) {
      sa.insert(t);
    }
    for (const auto &t : tb) {
      sb.insert(t);
    }
    const QSet<QString> &smaller = sa.size() <= sb.size() ? sa : sb;
    const QSet<QString> &bigger = sa.size() <= sb.size() ? sb : sa;
    if (!smaller.isEmpty() && bigger.contains(smaller)) {
      result = std::max(result, kSubsetFloor);
    }
    return result;
  }

  int TextMatch::bestCandidate(const QVector<SongCandidate> &candidates,
                               const QString &artist, const QString &title,
                               int duration_seconds) {
    const QString q_title = normalizeTitle(title);
    const QString q_artist = normalizeArtist(artist);
    const QString q_primary = normalizeArtist(primaryArtist(artist));
    const bool artist_known = !q_artist.isEmpty();

    int best = -1;
    double best_total = 0.0;
    for (int i = 0; i < candidates.size(); ++i) {
      const auto &c = candidates.at(i);
      if (c.instrumental) {
        continue;
      }
      const double title_score = similarity(q_title, normalizeTitle(c.title));
      if (title_score < kMinTitleScore) {
        continue;
      }
      double artist_score = kNeutralScore; // unknown query artist: neutral, floor waived
      if (artist_known) {
        const QString c_artist = normalizeArtist(c.artist);
        artist_score = std::max(similarity(q_artist, c_artist), similarity(q_primary, c_artist));
        if (artist_score < kMinArtistScore) {
          continue;
        }
      }
      double duration_score = kNeutralScore;
      if (duration_seconds > 0 && c.duration_seconds > 0) {
        const int d = std::abs(duration_seconds - c.duration_seconds);
        if (d <= kDurationExactSlack) {
          duration_score = 1.0;
        } else if (d <= kDurationMaxSlack) {
          duration_score = 1.0 - static_cast<double>(d - kDurationExactSlack) / (kDurationMaxSlack - kDurationExactSlack);
        } else {
          duration_score = 0.0;
        }
      }
      const double total = kTitleWeight * title_score
                         + kArtistWeight * artist_score
                         + kDurationWeight * duration_score
                         + (c.has_synced ? kSyncedTieBreak : 0.0);
      if (total >= kAcceptTotal && total > best_total) {
        best_total = total;
        best = i;
      }
    }
    return best;
  }
}
