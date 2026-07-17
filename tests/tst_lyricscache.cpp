#include <QtTest>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "lyrics/cache.h"

using Lyrics::Cache;
using Lyrics::TrackQuery;

class TestLyricsCache : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void cleanup();
  void missOnEmptyCache();
  void positiveRoundTrip();
  void survivesRestart();
  void keyCaseAndWhitespaceInsensitive();
  void keyIgnoresAlbumAndDuration();
  void keyIgnoresTitleDecorations();
  void distinctTracksStayDistinct();
  void emptyArtistOrTitleNeverCached();
  void miss_isRememberedForSameProviderSet();
  void miss_isNotSharedAcrossProviderSets();
  void miss_expiresAfterTtl();
  void clear_removesLyricsAndSentinels();
};

void TestLyricsCache::initTestCase() {
  // The cache lives under QStandardPaths::CacheLocation.
  QStandardPaths::setTestModeEnabled(true);
  QVERIFY(Cache::instance().dir().contains("downloaded_lyrics"));
}

void TestLyricsCache::cleanup() {
  Cache::instance().clear();
}

void TestLyricsCache::missOnEmptyCache() {
  Cache::Entry entry;
  QVERIFY(!Cache::instance().lookup({"Metallica", "One", "", 0}, entry));
}

void TestLyricsCache::positiveRoundTrip() {
  Cache::instance().storeFound({"Metallica", "One", "...And Justice for All", 447},
                               "lrclib", "Lyrics text");
  Cache::Entry entry;
  QVERIFY(Cache::instance().lookup({"Metallica", "One", "", 0}, entry));
  QCOMPARE(entry.provider, QStringLiteral("lrclib"));
  QCOMPARE(entry.lyrics, QStringLiteral("Lyrics text"));
}

// The whole point of moving off the session cache.
void TestLyricsCache::survivesRestart() {
  Cache::instance().storeFound({"Metallica", "One", "", 0}, "lrclib", "Lyrics text");
  // A fresh process would build a new Store over the same directory; the files
  // are the state, so re-reading through the same API proves persistence.
  const QString path = Cache::instance().dir() + Cache::key({"Metallica", "One", "", 0}) + ".json";
  QVERIFY(QFile::exists(path));
  Cache::Entry entry;
  QVERIFY(Cache::instance().lookup({"Metallica", "One", "", 0}, entry));
  QCOMPARE(entry.lyrics, QStringLiteral("Lyrics text"));
}

void TestLyricsCache::keyCaseAndWhitespaceInsensitive() {
  QCOMPARE(Cache::key({"Metallica", "One", "", 0}),
           Cache::key({"  metallica ", "ONE  ", "", 0}));
}

void TestLyricsCache::keyIgnoresAlbumAndDuration() {
  // The same song on another album or compilation should hit.
  QCOMPARE(Cache::key({"Metallica", "One", "...And Justice for All", 447}),
           Cache::key({"Metallica", "One", "S&M", 500}));
}

void TestLyricsCache::keyIgnoresTitleDecorations() {
  QCOMPARE(Cache::key({"Metallica", "One", "", 0}),
           Cache::key({"Metallica", "One (Remastered 2015)", "", 0}));
}

void TestLyricsCache::distinctTracksStayDistinct() {
  QVERIFY(Cache::key({"Metallica", "One", "", 0}) != Cache::key({"Metallica", "Two", "", 0}));
  QVERIFY(Cache::key({"Metallica", "One", "", 0}) != Cache::key({"Nirvana", "One", "", 0}));
}

void TestLyricsCache::emptyArtistOrTitleNeverCached() {
  QVERIFY(Cache::key({"", "One", "", 0}).isEmpty());
  QVERIFY(Cache::key({"Metallica", "", "", 0}).isEmpty());

  Cache::instance().storeFound({"", "One", "", 0}, "lrclib", "Lyrics text");
  Cache::Entry entry;
  QVERIFY(!Cache::instance().lookup({"", "One", "", 0}, entry));
}

void TestLyricsCache::miss_isRememberedForSameProviderSet() {
  const TrackQuery q{"Nobody", "Nothing", "", 0};
  const QStringList providers{"lrclib", "netease"};
  QVERIFY(!Cache::instance().isKnownMiss(q, providers));
  Cache::instance().storeNotFound(q, providers);
  QVERIFY(Cache::instance().isKnownMiss(q, providers));
  QVERIFY(Cache::instance().isKnownMiss(q, {"netease", "lrclib"}));
}

void TestLyricsCache::miss_isNotSharedAcrossProviderSets() {
  const TrackQuery q{"Nobody", "Nothing", "", 0};
  Cache::instance().storeNotFound(q, {"lrclib"});
  QVERIFY(Cache::instance().isKnownMiss(q, {"lrclib"}));
  QVERIFY(!Cache::instance().isKnownMiss(q, {"lrclib", "qq"}));
}

void TestLyricsCache::miss_expiresAfterTtl() {
  const TrackQuery q{"Nobody", "Nothing", "", 0};
  const QStringList providers{"lrclib"};
  Cache::instance().storeNotFound(q, providers);
  QVERIFY(Cache::instance().isKnownMiss(q, providers));

  QDir dir(Cache::instance().dir());
  const auto sentinels = dir.entryInfoList(QStringList() << "*.miss", QDir::Files);
  QCOMPARE(sentinels.size(), 1);
  const QString path = sentinels.first().absoluteFilePath();
  QFile f(path);
  QVERIFY(f.open(QIODevice::ReadWrite));
  QVERIFY(f.setFileTime(QDateTime::currentDateTime().addDays(-31), QFileDevice::FileModificationTime));
  f.close();

  QVERIFY(!Cache::instance().isKnownMiss(q, providers));
  QVERIFY(!QFile::exists(path));
}

void TestLyricsCache::clear_removesLyricsAndSentinels() {
  Cache::instance().storeFound({"Metallica", "One", "", 0}, "lrclib", "Lyrics text");
  Cache::instance().storeNotFound({"Nobody", "Nothing", "", 0}, {"lrclib"});
  QCOMPARE(Cache::instance().clear(), 2);
  Cache::Entry entry;
  QVERIFY(!Cache::instance().lookup({"Metallica", "One", "", 0}, entry));
  QVERIFY(!Cache::instance().isKnownMiss({"Nobody", "Nothing", "", 0}, {"lrclib"}));
}

QTEST_GUILESS_MAIN(TestLyricsCache)
#include "tst_lyricscache.moc"
