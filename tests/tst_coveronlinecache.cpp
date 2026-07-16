#include <QtTest>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QStandardPaths>

#include "coverart/online/cache.h"

using CoverArt::Online::AlbumQuery;
using CoverArt::Online::Cache;

class TestCoverOnlineCache : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void cleanup();
  void key_isStableAcrossCaseAndPadding();
  void key_ignoresAlbumDecorations();
  void key_emptyWhenArtistOrAlbumMissing();
  void lookup_missesWhenNothingStored();
  void storeFound_thenLookupHits();
  void storeFound_keepsBytesVerbatim();
  void storeFound_nothingWithoutKey();
  void miss_isRememberedForSameProviderSet();
  void miss_isNotSharedAcrossProviderSets();
  void miss_expiresAfterTtl();
  void clear_removesCoversAndSentinels();

private:
  static QByteArray samplePng();
};

void TestCoverOnlineCache::initTestCase() {
  // The cache lives under QStandardPaths::CacheLocation, which
  // MPZ_CONFIG_DIR_OVERRIDE does not affect.
  QStandardPaths::setTestModeEnabled(true);
  QVERIFY(Cache::instance().dir().contains("downloaded_covers"));
}

void TestCoverOnlineCache::cleanup() {
  Cache::instance().clear();
}

QByteArray TestCoverOnlineCache::samplePng() {
  QImage img(120, 120, QImage::Format_RGB32);
  img.fill(Qt::blue);
  QByteArray out;
  QBuffer buf(&out);
  buf.open(QIODevice::WriteOnly);
  img.save(&buf, "PNG");
  return out;
}

void TestCoverOnlineCache::key_isStableAcrossCaseAndPadding() {
  const QString a = Cache::key(AlbumQuery{"Pink Floyd", "The Wall"});
  const QString b = Cache::key(AlbumQuery{"  pink floyd ", "THE WALL"});
  QVERIFY(!a.isEmpty());
  QCOMPARE(a, b);
}

void TestCoverOnlineCache::key_ignoresAlbumDecorations() {
  const QString a = Cache::key(AlbumQuery{"Pink Floyd", "The Wall"});
  const QString b = Cache::key(AlbumQuery{"Pink Floyd", "The Wall (Remastered 2011)"});
  QCOMPARE(a, b);
}

void TestCoverOnlineCache::key_emptyWhenArtistOrAlbumMissing() {
  QVERIFY(Cache::key(AlbumQuery{"", "The Wall"}).isEmpty());
  QVERIFY(Cache::key(AlbumQuery{"Pink Floyd", ""}).isEmpty());
  QVERIFY(Cache::key(AlbumQuery{"", ""}).isEmpty());
}

void TestCoverOnlineCache::lookup_missesWhenNothingStored() {
  QVERIFY(Cache::instance().lookup(AlbumQuery{"Pink Floyd", "The Wall"}).isEmpty());
}

void TestCoverOnlineCache::storeFound_thenLookupHits() {
  const AlbumQuery q{"Pink Floyd", "The Wall"};
  const QString path = Cache::instance().storeFound(q, samplePng(), "png");
  QVERIFY(!path.isEmpty());
  QVERIFY(QFile::exists(path));
  QCOMPARE(Cache::instance().lookup(q), path);
  // Same album, differently decorated, still hits.
  QCOMPARE(Cache::instance().lookup(AlbumQuery{"pink floyd", "The Wall (Remastered 2011)"}), path);
}

// Unlike CoverArt::Embedded, which re-encodes everything to PNG.
void TestCoverOnlineCache::storeFound_keepsBytesVerbatim() {
  const AlbumQuery q{"Pink Floyd", "Animals"};
  const QByteArray bytes = samplePng();
  const QString path = Cache::instance().storeFound(q, bytes, "png");
  QVERIFY(path.endsWith(".png"));
  QFile f(path);
  QVERIFY(f.open(QIODevice::ReadOnly));
  QCOMPARE(f.readAll(), bytes);
}

void TestCoverOnlineCache::storeFound_nothingWithoutKey() {
  QVERIFY(Cache::instance().storeFound(AlbumQuery{"", ""}, samplePng(), "png").isEmpty());
  QVERIFY(Cache::instance().storeFound(AlbumQuery{"A", "B"}, QByteArray(), "png").isEmpty());
}

void TestCoverOnlineCache::miss_isRememberedForSameProviderSet() {
  const AlbumQuery q{"Nobody", "Nothing"};
  const QStringList providers{"deezer", "itunes"};
  QVERIFY(!Cache::instance().isKnownMiss(q, providers));
  Cache::instance().storeNotFound(q, providers);
  QVERIFY(Cache::instance().isKnownMiss(q, providers));
  // Order of the enabled set must not matter.
  QVERIFY(Cache::instance().isKnownMiss(q, {"itunes", "deezer"}));
}

// Enabling another provider has to retry rather than inherit the old miss.
void TestCoverOnlineCache::miss_isNotSharedAcrossProviderSets() {
  const AlbumQuery q{"Nobody", "Nothing"};
  Cache::instance().storeNotFound(q, {"deezer"});
  QVERIFY(Cache::instance().isKnownMiss(q, {"deezer"}));
  QVERIFY(!Cache::instance().isKnownMiss(q, {"deezer", "coverartarchive"}));
}

void TestCoverOnlineCache::miss_expiresAfterTtl() {
  const AlbumQuery q{"Nobody", "Nothing"};
  const QStringList providers{"deezer"};
  Cache::instance().storeNotFound(q, providers);
  QVERIFY(Cache::instance().isKnownMiss(q, providers));

  QDir dir(Cache::instance().dir());
  const auto sentinels = dir.entryInfoList(QStringList() << "*.miss", QDir::Files);
  QCOMPARE(sentinels.size(), 1);
  // 31 days old: past the 30-day TTL.
  const QString path = sentinels.first().absoluteFilePath();
  QFile f(path);
  QVERIFY(f.open(QIODevice::ReadWrite));
  QVERIFY(f.setFileTime(QDateTime::currentDateTime().addDays(-31), QFileDevice::FileModificationTime));
  f.close();

  QVERIFY(!Cache::instance().isKnownMiss(q, providers));
  // An expired sentinel is cleaned up rather than left to rot.
  QVERIFY(!QFile::exists(path));
}

void TestCoverOnlineCache::clear_removesCoversAndSentinels() {
  Cache::instance().storeFound(AlbumQuery{"Pink Floyd", "The Wall"}, samplePng(), "png");
  Cache::instance().storeNotFound(AlbumQuery{"Nobody", "Nothing"}, {"deezer"});
  QCOMPARE(Cache::instance().clear(), 2);
  QVERIFY(Cache::instance().lookup(AlbumQuery{"Pink Floyd", "The Wall"}).isEmpty());
  QVERIFY(!Cache::instance().isKnownMiss(AlbumQuery{"Nobody", "Nothing"}, {"deezer"}));
}

QTEST_GUILESS_MAIN(TestCoverOnlineCache)
#include "tst_coveronlinecache.moc"
