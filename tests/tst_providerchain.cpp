#include <QtTest>

#include "lyrics/providerchain.h"

using Lyrics::ProviderChain;

class TestProviderChain : public QObject {
  Q_OBJECT
private slots:
  void knownProvidersIsExactSet();
  void displayNameMapsKnownKeys();
  void displayNameEchoesUnknown();
};

void TestProviderChain::knownProvidersIsExactSet() {
  const QStringList expected{
    QStringLiteral("lrclib"),
    QStringLiteral("netease"),
    QStringLiteral("qq"),
    QStringLiteral("lyrics.ovh")};
  QCOMPARE(ProviderChain::knownProviders(), expected);
}

void TestProviderChain::displayNameMapsKnownKeys() {
  QCOMPARE(ProviderChain::displayName(QStringLiteral("lrclib")), QStringLiteral("LRCLIB"));
  QCOMPARE(ProviderChain::displayName(QStringLiteral("netease")), QStringLiteral("NetEase"));
  QCOMPARE(ProviderChain::displayName(QStringLiteral("qq")), QStringLiteral("QQ Music"));
  QCOMPARE(ProviderChain::displayName(QStringLiteral("lyrics.ovh")), QStringLiteral("Lyrics.ovh"));
}

void TestProviderChain::displayNameEchoesUnknown() {
  QCOMPARE(ProviderChain::displayName(QStringLiteral("unknown")), QStringLiteral("unknown"));
  QCOMPARE(ProviderChain::displayName(QString()), QString());
}

QTEST_GUILESS_MAIN(TestProviderChain)
#include "tst_providerchain.moc"
