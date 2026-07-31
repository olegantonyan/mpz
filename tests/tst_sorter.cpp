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
  void albumArtistOrdering();
  void discNumberOrdering();
  void discNumberOrderingIsNumeric();
  void discNumberOutranksTrackNumberInDefault();
  void sortVectorEndToEnd();
};

void TestSorter::defaultCriteriaIsCanonical() {
  const QString def = Sorter::defaultCriteria();
  QVERIFY(def.contains(QStringLiteral("YEAR")));
  QVERIFY(def.contains(QStringLiteral("ALBUM")));
  QVERIFY(def.contains(QStringLiteral("DIRECTORY")));
  QVERIFY(def.contains(QStringLiteral("DISCNUMBER")));
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

void TestSorter::albumArtistOrdering() {
  Sorter s(QStringLiteral("ALBUMARTIST"));
  Track a = mk({}, QStringLiteral("Zulu"), {}, {}, 0, 0);
  Track z = mk({}, QStringLiteral("Alpha"), {}, {}, 0, 0);
  a.setAlbumArtist(QStringLiteral("Aurora"));
  z.setAlbumArtist(QStringLiteral("Zenith"));
  QVERIFY(s.condition(a, z));
  QVERIFY(!s.condition(z, a));

  Sorter desc(QStringLiteral("-ALBUMARTIST"));
  QVERIFY(desc.condition(z, a));
}

void TestSorter::discNumberOrdering() {
  Sorter s(QStringLiteral("DISCNUMBER"));
  Track one = mk({}, {}, {}, {}, 0, 0);
  Track two = mk({}, {}, {}, {}, 0, 0);
  one.setDiscNumber(QStringLiteral("1/3"));
  two.setDiscNumber(QStringLiteral("2/3"));
  QVERIFY(s.condition(one, two));
  QVERIFY(!s.condition(two, one));

  Sorter desc(QStringLiteral("-DISCNUMBER"));
  QVERIFY(desc.condition(two, one));
}

void TestSorter::discNumberOrderingIsNumeric() {
  Sorter s(QStringLiteral("DISCNUMBER"));
  Track two = mk({}, {}, {}, {}, 0, 0);
  Track ten = mk({}, {}, {}, {}, 0, 0);
  two.setDiscNumber(QStringLiteral("2"));
  ten.setDiscNumber(QStringLiteral("10"));
  QVERIFY(s.condition(two, ten));
  QVERIFY(!s.condition(ten, two));
}

void TestSorter::discNumberOutranksTrackNumberInDefault() {
  Sorter s;
  Track disc1_track9 = mk(QStringLiteral("/a.flac"), {}, QStringLiteral("A"), {}, 9, 2024);
  Track disc2_track1 = mk(QStringLiteral("/a.flac"), {}, QStringLiteral("A"), {}, 1, 2024);
  disc1_track9.setDiscNumber(QStringLiteral("1"));
  disc2_track1.setDiscNumber(QStringLiteral("2"));
  QVERIFY(s.condition(disc1_track9, disc2_track1));
  QVERIFY(!s.condition(disc2_track1, disc1_track9));
}

void TestSorter::sortVectorEndToEnd() {
  Sorter s;
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

QTEST_GUILESS_MAIN(TestSorter)
#include "tst_sorter.moc"
