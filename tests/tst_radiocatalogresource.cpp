#include <QtTest>

#include "radio/catalog.h"

#include <QFile>
#include <QSet>
#include <QUrl>

using Radio::Catalog;

// Guards the shipped station list against a bad curation edit. Reads the source
// file directly rather than the qrc so the test needs no resource compilation.
class TestRadioCatalogResource : public QObject {
  Q_OBJECT
private slots:
  void shippedListParses();
  void somaFmStationsAreGrouped();
  void idsAreUnique();
  void everyUrlIsHttpAndQueryFree();
  void everyStationHasHomepage();
};

static Radio::Catalog shipped() {
  QFile f(QStringLiteral(RADIO_STATIONS_JSON));
  if (!f.open(QIODevice::ReadOnly)) {
    return Radio::Catalog();
  }
  return Radio::Catalog::fromJson(f.readAll());
}

void TestRadioCatalogResource::shippedListParses() {
  QFile f(QStringLiteral(RADIO_STATIONS_JSON));
  QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(QStringLiteral(RADIO_STATIONS_JSON)));
  QString error;
  const auto c = Catalog::fromJson(f.readAll(), &error);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QVERIFY(c.stations().size() >= 20);
}

// SomaFM channels live under one "SomaFM" folder; everything else is top-level.
void TestRadioCatalogResource::somaFmStationsAreGrouped() {
  const auto c = shipped();
  QCOMPARE(c.groups(), QStringList({"SomaFM"}));
  for (const auto &s : c.stations()) {
    const bool is_somafm = s.id.startsWith("somafm-");
    QCOMPARE(s.group == "SomaFM", is_somafm);
  }
}

void TestRadioCatalogResource::idsAreUnique() {
  const auto c = shipped();
  QSet<QString> seen;
  for (const auto &s : c.stations()) {
    QVERIFY2(!seen.contains(s.id), qPrintable(s.id));
    seen.insert(s.id);
  }
}

// Playback::Stream sends only url().path() and never follows redirects, so a
// query string is silently dropped and a non-http scheme cannot connect at all.
void TestRadioCatalogResource::everyUrlIsHttpAndQueryFree() {
  const auto c = shipped();
  for (const auto &s : c.stations()) {
    const QUrl parsed(s.url);
    QVERIFY2(parsed.scheme() == "http" || parsed.scheme() == "https", qPrintable(s.url));
    QVERIFY2(!parsed.hasQuery(), qPrintable(s.url));
    QVERIFY2(!parsed.path().isEmpty(), qPrintable(s.url));
  }
}

void TestRadioCatalogResource::everyStationHasHomepage() {
  for (const auto &s : shipped().stations()) {
    QVERIFY2(!s.homepage.isEmpty(), qPrintable(s.id));
  }
}

QTEST_MAIN(TestRadioCatalogResource)
#include "tst_radiocatalogresource.moc"
