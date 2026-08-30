#include <QtTest>
#include <QStandardPaths>

#include "lyrics/cache.h"
#include "lyrics/providerchain.h"

namespace {
  Lyrics::TrackQuery query(const QString &title) {
    Lyrics::TrackQuery q;
    q.artist = "artist";
    q.title = title;
    q.album = "album";
    return q;
  }
}

// Only the offline short-circuits: everything past them needs the network.
class TestProviderChainShortcuts : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void cleanup();
  void cacheHitEmitsSynchronouslyFromFetch();
  void cachedEntryWithoutLyricContentIsDropped();
  void emptyProviderListReportsNotFoundWithoutCachingIt();
  void unknownProviderNamesAreFilteredOut();
  void knownMissIsAnsweredFromTheCache();
};

void TestProviderChainShortcuts::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
}

void TestProviderChainShortcuts::cleanup() {
  Lyrics::Cache::instance().clear();
}

void TestProviderChainShortcuts::cacheHitEmitsSynchronouslyFromFetch() {
  const auto q = query("cached");
  Lyrics::Cache::instance().storeFound(q, "lrclib", "line one\nline two");

  Lyrics::ProviderChain chain;
  QSignalSpy found(&chain, &Lyrics::ProviderChain::found);

  chain.fetch(Lyrics::ProviderChain::knownProviders(), q);

  QCOMPARE(found.count(), 1);
  QCOMPARE(found.first().first().toString(), QString("lrclib"));
  QCOMPARE(found.first().at(1).toString(), QString("line one\nline two"));
}

void TestProviderChainShortcuts::cachedEntryWithoutLyricContentIsDropped() {
  const auto q = query("empty");
  Lyrics::Cache::instance().storeFound(q, "lrclib", "   \n\n  ");

  Lyrics::ProviderChain chain;
  QSignalSpy found(&chain, &Lyrics::ProviderChain::found);
  QSignalSpy not_found(&chain, &Lyrics::ProviderChain::notFound);

  chain.fetch({}, q);

  QCOMPARE(found.count(), 0);
  QCOMPARE(not_found.count(), 1);
}

void TestProviderChainShortcuts::emptyProviderListReportsNotFoundWithoutCachingIt() {
  const auto q = query("nobody");

  Lyrics::ProviderChain chain;
  QSignalSpy not_found(&chain, &Lyrics::ProviderChain::notFound);

  chain.fetch({}, q);

  QCOMPARE(not_found.count(), 1);
  // No provider was consulted, so enabling one later must not inherit a miss.
  QVERIFY(!Lyrics::Cache::instance().isKnownMiss(q, Lyrics::ProviderChain::knownProviders()));
}

void TestProviderChainShortcuts::unknownProviderNamesAreFilteredOut() {
  const QStringList known = Lyrics::ProviderChain::knownProviders();
  QVERIFY(!known.isEmpty());

  QCOMPARE(Lyrics::ProviderChain::filterKnown({"embedded", "sidecar"}), QStringList());
  QCOMPARE(Lyrics::ProviderChain::filterKnown({known.last(), "embedded", known.first()}),
           QStringList({known.last(), known.first()}));

  // A config full of retired built-in sources behaves like an empty list.
  Lyrics::ProviderChain chain;
  QSignalSpy not_found(&chain, &Lyrics::ProviderChain::notFound);
  chain.fetch({"embedded", "sidecar"}, query("legacy"));
  QCOMPARE(not_found.count(), 1);
}

void TestProviderChainShortcuts::knownMissIsAnsweredFromTheCache() {
  const auto q = query("missing");
  const QStringList providers = Lyrics::ProviderChain::knownProviders();
  Lyrics::Cache::instance().storeNotFound(q, providers);

  Lyrics::ProviderChain chain;
  QSignalSpy not_found(&chain, &Lyrics::ProviderChain::notFound);

  chain.fetch(providers, q);

  QCOMPARE(not_found.count(), 1);
}

QTEST_GUILESS_MAIN(TestProviderChainShortcuts)
#include "tst_providerchainshortcuts.moc"
