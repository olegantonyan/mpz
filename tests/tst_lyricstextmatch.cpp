#include <QtTest>

#include "lyrics/textmatch.h"

using Lyrics::SongCandidate;
using Lyrics::TextMatch;

class TestLyricsTextMatch : public QObject {
  Q_OBJECT
private slots:
  void normalizeTitleStripsRemasterParen();
  void normalizeTitleStripsDashSuffix();
  void normalizeTitleKeepsLegitParens();
  void normalizeStripsFeat();
  void normalizeFoldsUnicode();
  void normalizeEmptyAndOverstripGuard();
  void primaryArtistSplits();
  void similarityExactAndCase();
  void similarityTypo();
  void similarityTokenReorder();
  void similarityDisjoint();
  void similaritySubsetBonus();
  void bestCandidatePicksByDuration();
  void bestCandidateRejectsBelowThreshold();
  void bestCandidateSkipsInstrumental();
  void bestCandidatePrefersSyncedOnTie();
  void bestCandidateNeutralWithoutDuration();
  void bestCandidateEmptyQueryArtistWaivesFloor();
};

void TestLyricsTextMatch::normalizeTitleStripsRemasterParen() {
  QCOMPARE(TextMatch::normalizeTitle("Nothing Else Matters (Remastered 2021)"),
           QStringLiteral("Nothing Else Matters"));
  QCOMPARE(TextMatch::normalizeTitle("Song [Live]"), QStringLiteral("Song"));
  QCOMPARE(TextMatch::normalizeTitle("Song (Radio Edit)"), QStringLiteral("Song"));
}

void TestLyricsTextMatch::normalizeTitleStripsDashSuffix() {
  QCOMPARE(TextMatch::normalizeTitle("Hotel California - 2013 Remaster"),
           QStringLiteral("Hotel California"));
  QCOMPARE(TextMatch::normalizeTitle("X - Live - 2011 Remaster"), QStringLiteral("X"));
}

void TestLyricsTextMatch::normalizeTitleKeepsLegitParens() {
  QCOMPARE(TextMatch::normalizeTitle("Time (Clock of the Heart)"),
           QStringLiteral("Time (Clock of the Heart)"));
  QCOMPARE(TextMatch::normalizeTitle("Live and Let Die"), QStringLiteral("Live and Let Die"));
}

void TestLyricsTextMatch::normalizeStripsFeat() {
  QCOMPARE(TextMatch::normalizeTitle("Get Lucky (feat. Pharrell Williams)"),
           QStringLiteral("Get Lucky"));
  QCOMPARE(TextMatch::normalizeTitle("Song ft. Somebody"), QStringLiteral("Song"));
  QCOMPARE(TextMatch::normalizeArtist("Daft Punk feat. Pharrell Williams"),
           QStringLiteral("Daft Punk"));
}

void TestLyricsTextMatch::normalizeFoldsUnicode() {
  QCOMPARE(TextMatch::normalizeTitle("Don’t Stop Me Now"), QStringLiteral("Don't Stop Me Now"));
  QCOMPARE(TextMatch::normalizeTitle("Song — Part 2"), QStringLiteral("Song - Part 2"));
  QCOMPARE(TextMatch::normalizeTitle("  a \t b  "), QStringLiteral("a b"));
}

void TestLyricsTextMatch::normalizeEmptyAndOverstripGuard() {
  QCOMPARE(TextMatch::normalizeTitle("(Live)"), QStringLiteral("(Live)"));
  QCOMPARE(TextMatch::normalizeTitle(""), QString(""));
}

void TestLyricsTextMatch::primaryArtistSplits() {
  QCOMPARE(TextMatch::primaryArtist("A feat. B"), QStringLiteral("A"));
  QCOMPARE(TextMatch::primaryArtist("A & B"), QStringLiteral("A"));
  QCOMPARE(TextMatch::primaryArtist("A; B"), QStringLiteral("A"));
  QCOMPARE(TextMatch::primaryArtist("Metallica"), QStringLiteral("Metallica"));
}

void TestLyricsTextMatch::similarityExactAndCase() {
  QCOMPARE(TextMatch::similarity("Bohemian Rhapsody", "bohemian rhapsody"), 1.0);
}

void TestLyricsTextMatch::similarityTypo() {
  QVERIFY(TextMatch::similarity("Bohemian Rapsody", "Bohemian Rhapsody") > 0.85);
}

void TestLyricsTextMatch::similarityTokenReorder() {
  QVERIFY(TextMatch::similarity("Hendrix Jimi", "Jimi Hendrix") > 0.85);
}

void TestLyricsTextMatch::similarityDisjoint() {
  QVERIFY(TextMatch::similarity("Hello", "Zebra Quux") < 0.3);
}

void TestLyricsTextMatch::similaritySubsetBonus() {
  QVERIFY(TextMatch::similarity("Nothing Else Matters",
                                "Nothing Else Matters Remastered 2021") >= 0.85);
}

void TestLyricsTextMatch::bestCandidatePicksByDuration() {
  const QVector<SongCandidate> candidates = {
    {"Nothing Else Matters", "Metallica", 200, false, false},
    {"Nothing Else Matters", "Metallica", 260, false, false},
  };
  QCOMPARE(TextMatch::bestCandidate(candidates, "Metallica", "Nothing Else Matters", 201), 0);
  QCOMPARE(TextMatch::bestCandidate(candidates, "Metallica", "Nothing Else Matters", 259), 1);
}

void TestLyricsTextMatch::bestCandidateRejectsBelowThreshold() {
  const QVector<SongCandidate> candidates = {
    {"Completely Different Song", "Someone Else", 200, false, false},
  };
  QCOMPARE(TextMatch::bestCandidate(candidates, "Metallica", "Nothing Else Matters", 200), -1);
}

void TestLyricsTextMatch::bestCandidateSkipsInstrumental() {
  const QVector<SongCandidate> candidates = {
    {"Nothing Else Matters", "Metallica", 200, false, true},
  };
  QCOMPARE(TextMatch::bestCandidate(candidates, "Metallica", "Nothing Else Matters", 200), -1);
}

void TestLyricsTextMatch::bestCandidatePrefersSyncedOnTie() {
  const QVector<SongCandidate> candidates = {
    {"Nothing Else Matters", "Metallica", 200, false, false},
    {"Nothing Else Matters", "Metallica", 200, true, false},
  };
  QCOMPARE(TextMatch::bestCandidate(candidates, "Metallica", "Nothing Else Matters", 200), 1);
}

void TestLyricsTextMatch::bestCandidateNeutralWithoutDuration() {
  const QVector<SongCandidate> candidates = {
    {"Nothing Else Matters", "Metallica", 0, false, false},
  };
  QCOMPARE(TextMatch::bestCandidate(candidates, "Metallica", "Nothing Else Matters", 200), 0);
}

void TestLyricsTextMatch::bestCandidateEmptyQueryArtistWaivesFloor() {
  const QVector<SongCandidate> candidates = {
    {"Nothing Else Matters", "Metallica", 200, false, false},
  };
  QCOMPARE(TextMatch::bestCandidate(candidates, "", "Nothing Else Matters", 200), 0);
}

QTEST_GUILESS_MAIN(TestLyricsTextMatch)
#include "tst_lyricstextmatch.moc"
