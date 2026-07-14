#include <QtTest>

#include "playlist/sorter.h"
#include "track.h"

#include <algorithm>

using Playlist::Sorter;

namespace {
  Track mk(const QString &filepath,
           const QString &artist,
           const QString &album,
           const QString &title,
           quint16 tracknum,
           quint16 year) {
    return Track(filepath, 0, artist, album, title, tracknum, year, 0, 0, 0, 0);
  }
}

class TestSorter : public QObject {
  Q_OBJECT
private slots:
  void defaultCriteriaIsCanonical();
  void singleCriterionAscending();
  void descendingPrefix();
  void chainBreaksOnFirstNonEqual();
  void emptyCriteriaNeverOrders();
  void whitespaceCriterionIsParsed();
  void unknownCriterionIsIgnored();
  void titleArtistAlbumFilenameOrderings();
  void sortVectorEndToEnd();
  void albumArtistOrdering();
  void genreOrdering();
  void discNumberOrdering();
  void discNumberBeforeTrackNumber();
};

void TestSorter::defaultCriteriaIsCanonical() {
  const QString def = Sorter::defaultCriteria();
  QVERIFY(def.contains(QStringLiteral("YEAR")));
  QVERIFY(def.contains(QStringLiteral("ALBUM")));
  QVERIFY(def.contains(QStringLiteral("DIRECTORY")));
  QVERIFY(def.contains(QStringLiteral("TRACKNUMBER")));
  QVERIFY(def.contains(QStringLiteral("FILENAME")));
  QVERIFY(def.contains(QStringLiteral("TITLE")));
}

void TestSorter::singleCriterionAscending() {
  Sorter s(QStringLiteral("YEAR"));
  Track older = mk(QStringLiteral("/a.flac"), {}, {}, {}, 0, 2000);
  Track newer = mk(QStringLiteral("/b.flac"), {}, {}, {}, 0, 2024);
  QVERIFY(s.condition(older, newer));
  QVERIFY(!s.condition(newer, older));
  QVERIFY(!s.condition(older, older));
}

void TestSorter::descendingPrefix() {
  Sorter s(QStringLiteral("-YEAR"));
  Track older = mk(QStringLiteral("/a.flac"), {}, {}, {}, 0, 2000);
  Track newer = mk(QStringLiteral("/b.flac"), {}, {}, {}, 0, 2024);
  QVERIFY(s.condition(newer, older));
  QVERIFY(!s.condition(older, newer));
}

void TestSorter::chainBreaksOnFirstNonEqual() {
  Sorter s(QStringLiteral("YEAR / TRACKNUMBER"));
  Track a = mk({}, {}, {}, {}, 5, 2024);
  Track b = mk({}, {}, {}, {}, 1, 2024);
  // Same year; tracknumber decides. lower track number sorts first.
  QVERIFY(s.condition(b, a));
  QVERIFY(!s.condition(a, b));
}

void TestSorter::emptyCriteriaNeverOrders() {
  Sorter s(QStringLiteral(""));
  Track a = mk(QStringLiteral("/a.flac"), QStringLiteral("Z"), {}, {}, 0, 2024);
  Track b = mk(QStringLiteral("/b.flac"), QStringLiteral("A"), {}, {}, 0, 2000);
  QVERIFY(!s.condition(a, b));
  QVERIFY(!s.condition(b, a));
}

void TestSorter::whitespaceCriterionIsParsed() {
  Sorter s(QStringLiteral("   year   "));
  Track older = mk({}, {}, {}, {}, 0, 1999);
  Track newer = mk({}, {}, {}, {}, 0, 2010);
  QVERIFY(s.condition(older, newer));
}

void TestSorter::unknownCriterionIsIgnored() {
  Sorter s(QStringLiteral("NONSENSE / YEAR"));
  Track older = mk({}, {}, {}, {}, 0, 1999);
  Track newer = mk({}, {}, {}, {}, 0, 2010);
  QVERIFY(s.condition(older, newer));
}

void TestSorter::titleArtistAlbumFilenameOrderings() {
  Sorter byArtist(QStringLiteral("ARTIST"));
  Track a = mk(QStringLiteral("/a.flac"), QStringLiteral("Alpha"), {}, {}, 0, 0);
  Track z = mk(QStringLiteral("/z.flac"), QStringLiteral("Zulu"),  {}, {}, 0, 0);
  QVERIFY(byArtist.condition(a, z));

  Sorter byTitle(QStringLiteral("TITLE"));
  Track ta = mk({}, {}, {}, QStringLiteral("Anchor"), 0, 0);
  Track tz = mk({}, {}, {}, QStringLiteral("Zenith"), 0, 0);
  QVERIFY(byTitle.condition(ta, tz));

  Sorter byAlbum(QStringLiteral("ALBUM"));
  Track la = mk({}, {}, QStringLiteral("Aurora"), {}, 0, 0);
  Track lz = mk({}, {}, QStringLiteral("Zenith"), {}, 0, 0);
  QVERIFY(byAlbum.condition(la, lz));

  Sorter byFilename(QStringLiteral("FILENAME"));
  Track fa = mk(QStringLiteral("/dir/a.flac"), {}, {}, {}, 0, 0);
  Track fz = mk(QStringLiteral("/dir/z.flac"), {}, {}, {}, 0, 0);
  QVERIFY(byFilename.condition(fa, fz));
}

void TestSorter::sortVectorEndToEnd() {
  Sorter s; // default criteria: YEAR / ALBUM / DIRECTORY / TRACKNUMBER / FILENAME / TITLE
  QVector<Track> v;
  v << mk(QStringLiteral("/a.flac"), {}, QStringLiteral("Z"), {}, 0, 2010);
  v << mk(QStringLiteral("/b.flac"), {}, QStringLiteral("A"), {}, 0, 1990);
  v << mk(QStringLiteral("/c.flac"), {}, QStringLiteral("M"), {}, 0, 2000);
  std::sort(v.begin(), v.end(), [&s](const Track &x, const Track &y) {
    return s.condition(x, y);
  });
  QCOMPARE(v.at(0).year(), quint16{1990});
  QCOMPARE(v.at(1).year(), quint16{2000});
  QCOMPARE(v.at(2).year(), quint16{2010});
}

void TestSorter::albumArtistOrdering() {
  Sorter s(QStringLiteral("ALBUMARTIST"));
  Track a = mk({}, {}, {}, {}, 0, 0);
  Track b = mk({}, {}, {}, {}, 0, 0);
  a.setAlbumArtist(QStringLiteral("Aphex Twin"));
  b.setAlbumArtist(QStringLiteral("Boards of Canada"));
  QVERIFY(s.condition(a, b));
  QVERIFY(!s.condition(b, a));

  Sorter desc(QStringLiteral("-ALBUMARTIST"));
  QVERIFY(desc.condition(b, a));
}

void TestSorter::genreOrdering() {
  Sorter s(QStringLiteral("GENRE"));
  Track a = mk({}, {}, {}, {}, 0, 0);
  Track b = mk({}, {}, {}, {}, 0, 0);
  a.setGenre(QStringLiteral("Ambient"));
  b.setGenre(QStringLiteral("Rock"));
  QVERIFY(s.condition(a, b));
  QVERIFY(!s.condition(b, a));
  QVERIFY(!s.condition(a, a));
}

void TestSorter::discNumberOrdering() {
  Sorter s(QStringLiteral("DISCNUMBER"));
  Track d1 = mk({}, {}, {}, {}, 0, 0);
  Track d2 = mk({}, {}, {}, {}, 0, 0);
  d1.setDiscNumber(1);
  d2.setDiscNumber(2);
  QVERIFY(s.condition(d1, d2));
  QVERIFY(!s.condition(d2, d1));
  QVERIFY(!s.condition(d1, d1));

  Sorter desc(QStringLiteral("-DISCNUMBER"));
  QVERIFY(desc.condition(d2, d1));
}

// The multi-disc fix: disc 2 track 1 must not sort beside disc 1 track 1.
void TestSorter::discNumberBeforeTrackNumber() {
  Sorter s(QStringLiteral("DISCNUMBER / TRACKNUMBER"));
  Track d1t1 = mk(QStringLiteral("/d1t1.flac"), {}, {}, {}, 1, 0);
  Track d1t2 = mk(QStringLiteral("/d1t2.flac"), {}, {}, {}, 2, 0);
  Track d2t1 = mk(QStringLiteral("/d2t1.flac"), {}, {}, {}, 1, 0);
  d1t1.setDiscNumber(1);
  d1t2.setDiscNumber(1);
  d2t1.setDiscNumber(2);

  QVector<Track> v{d2t1, d1t2, d1t1};
  std::sort(v.begin(), v.end(), [&s](const Track &a, const Track &b) {
    return s.condition(a, b);
  });
  QCOMPARE(v.at(0).filename(), QStringLiteral("d1t1.flac"));
  QCOMPARE(v.at(1).filename(), QStringLiteral("d1t2.flac"));
  QCOMPARE(v.at(2).filename(), QStringLiteral("d2t1.flac"));
}

QTEST_GUILESS_MAIN(TestSorter)
#include "tst_sorter.moc"
