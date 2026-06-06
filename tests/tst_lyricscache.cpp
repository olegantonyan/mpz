#include <QtTest>

#include "lyrics/cache.h"

using Lyrics::Cache;
using Lyrics::TrackQuery;

class TestLyricsCache : public QObject {
  Q_OBJECT
private slots:
  void missOnEmptyCache();
  void positiveRoundTrip();
  void negativeRoundTrip();
  void keyCaseAndWhitespaceInsensitive();
  void keyIgnoresAlbumAndDuration();
  void distinctTracksStayDistinct();
  void emptyArtistOrTitleNeverCached();
  void positiveOverwritesNegative();
};

void TestLyricsCache::missOnEmptyCache() {
  Cache cache;
  Cache::Entry entry;
  QVERIFY(!cache.lookup({"Metallica", "One", "", 0}, entry));
}

void TestLyricsCache::positiveRoundTrip() {
  Cache cache;
  cache.storeFound({"Metallica", "One", "...And Justice for All", 447}, "lrclib", "Lyrics text");
  Cache::Entry entry;
  QVERIFY(cache.lookup({"Metallica", "One", "...And Justice for All", 447}, entry));
  QVERIFY(entry.found);
  QCOMPARE(entry.provider, QStringLiteral("lrclib"));
  QCOMPARE(entry.lyrics, QStringLiteral("Lyrics text"));
}

void TestLyricsCache::negativeRoundTrip() {
  Cache cache;
  cache.storeNotFound({"Obscure Artist", "Unknown Song", "", 0});
  Cache::Entry entry;
  QVERIFY(cache.lookup({"Obscure Artist", "Unknown Song", "", 0}, entry));
  QVERIFY(!entry.found);
}

void TestLyricsCache::keyCaseAndWhitespaceInsensitive() {
  QCOMPARE(Cache::key({" Metallica ", "ONE", "", 0}),
           Cache::key({"metallica", " one ", "", 0}));

  Cache cache;
  cache.storeFound({"Metallica", "One", "", 0}, "lrclib", "Lyrics text");
  Cache::Entry entry;
  QVERIFY(cache.lookup({"  METALLICA", "one  ", "", 0}, entry));
  QVERIFY(entry.found);
}

void TestLyricsCache::keyIgnoresAlbumAndDuration() {
  QCOMPARE(Cache::key({"Metallica", "One", "...And Justice for All", 447}),
           Cache::key({"Metallica", "One", "S&M", 465}));
}

void TestLyricsCache::distinctTracksStayDistinct() {
  QVERIFY(Cache::key({"Metallica", "One", "", 0}) != Cache::key({"Metallica", "Two", "", 0}));
  QVERIFY(Cache::key({"Metallica", "One", "", 0}) != Cache::key({"U2", "One", "", 0}));

  Cache cache;
  cache.storeFound({"Metallica", "One", "", 0}, "lrclib", "Lyrics text");
  Cache::Entry entry;
  QVERIFY(!cache.lookup({"U2", "One", "", 0}, entry));
}

void TestLyricsCache::emptyArtistOrTitleNeverCached() {
  QVERIFY(Cache::key({"", "One", "", 0}).isEmpty());
  QVERIFY(Cache::key({"Metallica", "  ", "", 0}).isEmpty());

  Cache cache;
  cache.storeFound({"", "One", "", 0}, "lrclib", "Lyrics text");
  cache.storeNotFound({"Metallica", "", "", 0});
  Cache::Entry entry;
  QVERIFY(!cache.lookup({"", "One", "", 0}, entry));
  QVERIFY(!cache.lookup({"Metallica", "", "", 0}, entry));
}

void TestLyricsCache::positiveOverwritesNegative() {
  Cache cache;
  cache.storeNotFound({"Metallica", "One", "", 0});
  cache.storeFound({"Metallica", "One", "", 0}, "netease", "Lyrics text");
  Cache::Entry entry;
  QVERIFY(cache.lookup({"Metallica", "One", "", 0}, entry));
  QVERIFY(entry.found);
  QCOMPARE(entry.provider, QStringLiteral("netease"));
}

QTEST_GUILESS_MAIN(TestLyricsCache)
#include "tst_lyricscache.moc"
