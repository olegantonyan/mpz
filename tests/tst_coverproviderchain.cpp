#include <QtTest>

#include "coverart/online/providerchain.h"

using CoverArt::Online::ProviderChain;

class TestCoverProviderChain : public QObject {
  Q_OBJECT
private slots:
  void knownProvidersIsExactSet();
  void displayNameMapsKnownKeys();
  void displayNameEchoesUnknown();
  void filterKnownKeepsOrderAndDropsUnknown();
};

void TestCoverProviderChain::knownProvidersIsExactSet() {
  // Cover Art Archive stays last: two round-trips and a 1 req/sec limit.
  const QStringList expected{
    QStringLiteral("deezer"),
    QStringLiteral("itunes"),
    QStringLiteral("coverartarchive")};
  QCOMPARE(ProviderChain::knownProviders(), expected);
}

void TestCoverProviderChain::displayNameMapsKnownKeys() {
  QCOMPARE(ProviderChain::displayName(QStringLiteral("deezer")), QStringLiteral("Deezer"));
  QCOMPARE(ProviderChain::displayName(QStringLiteral("itunes")), QStringLiteral("Apple Music"));
  QCOMPARE(ProviderChain::displayName(QStringLiteral("coverartarchive")), QStringLiteral("Cover Art Archive"));
}

void TestCoverProviderChain::displayNameEchoesUnknown() {
  QCOMPARE(ProviderChain::displayName(QStringLiteral("unknown")), QStringLiteral("unknown"));
  QCOMPARE(ProviderChain::displayName(QString()), QString());
}

void TestCoverProviderChain::filterKnownKeepsOrderAndDropsUnknown() {
  const QStringList input{"itunes", "embedded", "deezer", "nonsense"};
  const QStringList expected{"itunes", "deezer"};
  QCOMPARE(ProviderChain::filterKnown(input), expected);
  QVERIFY(ProviderChain::filterKnown({}).isEmpty());
}

QTEST_GUILESS_MAIN(TestCoverProviderChain)
#include "tst_coverproviderchain.moc"
