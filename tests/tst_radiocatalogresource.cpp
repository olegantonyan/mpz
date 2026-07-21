#include <QtTest>

#include "radio/catalog.h"

#include <QFile>
#include <QSet>
#include <QUrl>

// Guards the shipped station list against a bad curation edit. Reads the source
// file directly rather than the qrc so the test needs no resource compilation.
class TestRadioCatalogResource : public QObject {
  Q_OBJECT
private slots:
  void shippedListParses();
  void stationsAreGroupedByProvider();
  void idsAreUnique();
  void everyUrlIsHttpAndQueryFree();
  void everyStationHasHomepage();
};

static QVector<Radio::Station> shipped() {
  QFile f(QStringLiteral(RADIO_STATIONS_JSON));
  if (!f.open(QIODevice::ReadOnly)) {
    return {};
  }
  return Radio::Catalog::fromJson(f.readAll());
}

void TestRadioCatalogResource::shippedListParses() {
  QFile f(QStringLiteral(RADIO_STATIONS_JSON));
  QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(QStringLiteral(RADIO_STATIONS_JSON)));
  QString error;
  const auto stations = Radio::Catalog::fromJson(f.readAll(), &error);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QVERIFY(stations.size() >= 20);
}

// Multi-station providers live under one folder; single stations stay top-level.
// A station's group is a pure function of its id prefix.
void TestRadioCatalogResource::stationsAreGroupedByProvider() {
  const auto stations = shipped();
  QCOMPARE(Radio::Catalog::groups(stations),
           QStringList({"SomaFM", "Radio Paradise", "FIP", "France Musique", "Nightride FM"}));
  for (const auto &s : stations) {
    QString expected;
    if (s.id.startsWith("somafm-")) {
      expected = "SomaFM";
    } else if (s.id.startsWith("radioparadise-")) {
      expected = "Radio Paradise";
    } else if (s.id.startsWith("fip")) {
      expected = "FIP";
    } else if (s.id.startsWith("francemusique-")) {
      expected = "France Musique";
    } else if (s.id.startsWith("nightride-")) {
      expected = "Nightride FM";
    }
    QCOMPARE(s.group, expected);
  }
}

void TestRadioCatalogResource::idsAreUnique() {
  QSet<QString> seen;
  for (const auto &s : shipped()) {
    QVERIFY2(!seen.contains(s.id), qPrintable(s.id));
    seen.insert(s.id);
  }
}

// Playback::Stream sends only url().path() and never follows redirects, so a
// query string is silently dropped and a non-http scheme cannot connect at all.
void TestRadioCatalogResource::everyUrlIsHttpAndQueryFree() {
  for (const auto &s : shipped()) {
    const QUrl parsed(s.url);
    QVERIFY2(parsed.scheme() == "http" || parsed.scheme() == "https", qPrintable(s.url));
    QVERIFY2(!parsed.hasQuery(), qPrintable(s.url));
    QVERIFY2(!parsed.path().isEmpty(), qPrintable(s.url));
  }
}

void TestRadioCatalogResource::everyStationHasHomepage() {
  for (const auto &s : shipped()) {
    QVERIFY2(!s.homepage.isEmpty(), qPrintable(s.id));
  }
}

QTEST_MAIN(TestRadioCatalogResource)
#include "tst_radiocatalogresource.moc"
