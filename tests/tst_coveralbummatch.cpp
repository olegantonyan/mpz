#include <QtTest>

#include "coverart/online/albummatch.h"

using CoverArt::Online::AlbumCandidate;
using CoverArt::Online::AlbumMatch;

class TestCoverAlbumMatch : public QObject {
  Q_OBJECT
private slots:
  void exactMatchWins();
  void picksBestOfSeveral();
  void toleratesDecoratedAlbumTitles();
  void toleratesFeaturedArtists();
  void rejectsWrongAlbum();
  void rejectsWrongArtist();
  void rejectsEmptyFields();
  void emptyCandidatesYieldNoMatch();
};

void TestCoverAlbumMatch::exactMatchWins() {
  const QVector<AlbumCandidate> candidates{{"Pink Floyd", "The Wall"}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Pink Floyd", "The Wall"), 0);
}

void TestCoverAlbumMatch::picksBestOfSeveral() {
  const QVector<AlbumCandidate> candidates{
    {"Pink Floyd", "The Division Bell"},
    {"Pink Floyd", "The Wall"},
    {"Pink Floyd", "Animals"}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Pink Floyd", "The Wall"), 1);
}

void TestCoverAlbumMatch::toleratesDecoratedAlbumTitles() {
  const QVector<AlbumCandidate> candidates{{"Pink Floyd", "The Wall (Remastered 2011)"}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Pink Floyd", "The Wall"), 0);
}

void TestCoverAlbumMatch::toleratesFeaturedArtists() {
  const QVector<AlbumCandidate> candidates{{"Queen", "The Works"}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Queen feat. David Bowie", "The Works"), 0);
}

// A wrong cover is written to disk and outlives the session, so a near miss
// must lose rather than "probably match".
void TestCoverAlbumMatch::rejectsWrongAlbum() {
  const QVector<AlbumCandidate> candidates{{"Pink Floyd", "Animals"}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Pink Floyd", "The Wall"), -1);
}

void TestCoverAlbumMatch::rejectsWrongArtist() {
  const QVector<AlbumCandidate> candidates{{"Some Tribute Band", "The Wall"}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Pink Floyd", "The Wall"), -1);
}

void TestCoverAlbumMatch::rejectsEmptyFields() {
  const QVector<AlbumCandidate> candidates{{"Pink Floyd", ""}};
  QCOMPARE(AlbumMatch::bestCandidate(candidates, "Pink Floyd", "The Wall"), -1);
  QCOMPARE(AlbumMatch::bestCandidate({{"Pink Floyd", "The Wall"}}, "Pink Floyd", ""), -1);
}

void TestCoverAlbumMatch::emptyCandidatesYieldNoMatch() {
  QCOMPARE(AlbumMatch::bestCandidate({}, "Pink Floyd", "The Wall"), -1);
}

QTEST_GUILESS_MAIN(TestCoverAlbumMatch)
#include "tst_coveralbummatch.moc"
