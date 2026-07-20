#include <QtTest>

#include "radio/catalog.h"

using Radio::Catalog;
using Radio::Station;

namespace {
  QByteArray oneStation(const QString &fields) {
    return QString(R"({"stations":[{"id":"a","name":"A",%1}]})").arg(fields).toUtf8();
  }

  const char *kUrl = R"("url":"https://h/a","codec":"mp3","bitrate":256)";
}

class TestRadioCatalog : public QObject {
  Q_OBJECT
private slots:
  void parsesStationFields();
  void rejectsInvalidJson();
  void rejectsNonObjectRoot();
  void rejectsMissingStationsArray();
  void rejectsStationWithoutId();
  void rejectsStationWithoutUrl();
  void rejectsNonHttpUrl();
  void acceptsHttpUrl();
  void rejectsDuplicateIds();
  void clearsErrorOnSuccess();
  void groupsAreOrderedAndDeduped();
  void ungroupedStationsAreNotInGroups();
  void byIdFindsAndMisses();
  void extendsBuiltinDefaultsFalse();
  void extendsBuiltinIsRead();
  void subtitleFormatsCodecAndDescription();
  void subtitleFallsBackToDescription();
  void subtitleCodecOnlyWhenNoDescription();
};

void TestRadioCatalog::parsesStationFields() {
  const auto json = QByteArray(R"({"stations":[{
    "id":"somafm-lush","name":"Lush","group":"SomaFM","description":"Mellow vocals.",
    "url":"https://h/lush-128-mp3","codec":"mp3","bitrate":128,
    "homepage":"https://somafm.com/lush/","logo_url":"https://h/l.png"}]})");
  QString error;
  const auto c = Catalog::fromJson(json, &error);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QCOMPARE(c.stations().size(), 1);

  const auto &s = c.stations().first();
  QCOMPARE(s.id, QStringLiteral("somafm-lush"));
  QCOMPARE(s.name, QStringLiteral("Lush"));
  QCOMPARE(s.group, QStringLiteral("SomaFM"));
  QCOMPARE(s.description, QStringLiteral("Mellow vocals."));
  QCOMPARE(s.url, QStringLiteral("https://h/lush-128-mp3"));
  QCOMPARE(s.codec, QStringLiteral("mp3"));
  QCOMPARE(s.bitrate, quint16{128});
  QCOMPARE(s.homepage, QStringLiteral("https://somafm.com/lush/"));
  QCOMPARE(s.logo_url, QStringLiteral("https://h/l.png"));
}

void TestRadioCatalog::rejectsInvalidJson() {
  QString error;
  const auto c = Catalog::fromJson("{\"stations\":[", &error);
  QVERIFY(!error.isEmpty());
  QVERIFY(c.stations().isEmpty());
}

void TestRadioCatalog::rejectsNonObjectRoot() {
  QString error;
  Catalog::fromJson("[]", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsMissingStationsArray() {
  QString error;
  Catalog::fromJson("{\"version\":1}", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsStationWithoutId() {
  QString error;
  Catalog::fromJson(R"({"stations":[{"name":"A","url":"https://h/a"}]})", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsStationWithoutUrl() {
  QString error;
  Catalog::fromJson(R"({"stations":[{"id":"a","name":"A"}]})", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsNonHttpUrl() {
  QString error;
  Catalog::fromJson(oneStation(R"("url":"ftp://h/a")"), &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::acceptsHttpUrl() {
  QString error;
  const auto c = Catalog::fromJson(
    oneStation(R"("url":"http://relay4.slayradio.org:8000/","codec":"mp3","bitrate":128)"), &error);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QCOMPARE(c.stations().size(), 1);
}

void TestRadioCatalog::rejectsDuplicateIds() {
  QString error;
  Catalog::fromJson(QString(R"({"stations":[)"
                            R"({"id":"a","name":"A",%1},)"
                            R"({"id":"a","name":"B",%1}]})")
                      .arg(kUrl).toUtf8(), &error);
  QVERIFY(!error.isEmpty());
  QVERIFY(error.contains("a"));
}

void TestRadioCatalog::clearsErrorOnSuccess() {
  QString error = QStringLiteral("stale");
  Catalog::fromJson(oneStation(kUrl), &error);
  QVERIFY(error.isEmpty());
}

void TestRadioCatalog::groupsAreOrderedAndDeduped() {
  const auto json = QString(R"({"stations":[)"
                            R"({"id":"a","name":"A","group":"Zed",%1},)"
                            R"({"id":"b","name":"B","group":"Alpha",%1},)"
                            R"({"id":"c","name":"C","group":"Zed",%1}]})")
                      .arg(kUrl).toUtf8();
  const auto c = Catalog::fromJson(json);
  QCOMPARE(c.groups(), QStringList({"Zed", "Alpha"}));
}

void TestRadioCatalog::ungroupedStationsAreNotInGroups() {
  const auto c = Catalog::fromJson(oneStation(kUrl));
  QVERIFY(c.stations().first().group.isEmpty());
  QVERIFY(c.groups().isEmpty());
}

void TestRadioCatalog::byIdFindsAndMisses() {
  const auto c = Catalog::fromJson(oneStation(kUrl));
  QVERIFY(c.byId("a") != nullptr);
  QCOMPARE(c.byId("a")->name, QStringLiteral("A"));
  QCOMPARE(c.byId("nope"), nullptr);
}

void TestRadioCatalog::extendsBuiltinDefaultsFalse() {
  QVERIFY(!Catalog::fromJson(oneStation(kUrl)).extendsBuiltin());
}

void TestRadioCatalog::extendsBuiltinIsRead() {
  const auto json = QString(R"({"extends_builtin":true,"stations":[)"
                            R"({"id":"a","name":"A",%1}]})").arg(kUrl).toUtf8();
  QVERIFY(Catalog::fromJson(json).extendsBuiltin());
}

void TestRadioCatalog::subtitleFormatsCodecAndDescription() {
  Station s;
  s.codec = QStringLiteral("mp3");
  s.bitrate = 256;
  s.description = QStringLiteral("Chilled beats.");
  QCOMPARE(s.subtitle(), QStringLiteral("MP3 256k · Chilled beats."));
}

void TestRadioCatalog::subtitleFallsBackToDescription() {
  Station s;
  s.description = QStringLiteral("Just words.");
  QCOMPARE(s.subtitle(), QStringLiteral("Just words."));
}

void TestRadioCatalog::subtitleCodecOnlyWhenNoDescription() {
  Station s;
  s.codec = QStringLiteral("aac");
  s.bitrate = 128;
  QCOMPARE(s.subtitle(), QStringLiteral("AAC 128k"));
}

QTEST_MAIN(TestRadioCatalog)
#include "tst_radiocatalog.moc"
