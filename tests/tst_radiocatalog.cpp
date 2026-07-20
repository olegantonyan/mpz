#include <QtTest>

#include "radio/catalog.h"

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
  void ungroupedStationsHaveNoGroups();
  void subtitleFormatsCodecAndBitrate();
  void subtitleCodecOnlyWhenNoBitrate();
  void subtitleEmptyWhenNoCodec();
};

void TestRadioCatalog::parsesStationFields() {
  const auto json = QByteArray(R"({"stations":[{
    "id":"somafm-lush","name":"Lush","group":"SomaFM",
    "url":"https://h/lush-128-mp3","codec":"mp3","bitrate":128,
    "homepage":"https://somafm.com/lush/"}]})");
  QString error;
  const auto stations = Radio::Catalog::fromJson(json, &error);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QCOMPARE(stations.size(), 1);

  const auto &s = stations.first();
  QCOMPARE(s.id, QStringLiteral("somafm-lush"));
  QCOMPARE(s.name, QStringLiteral("Lush"));
  QCOMPARE(s.group, QStringLiteral("SomaFM"));
  QCOMPARE(s.url, QStringLiteral("https://h/lush-128-mp3"));
  QCOMPARE(s.codec, QStringLiteral("mp3"));
  QCOMPARE(s.bitrate, quint16{128});
  QCOMPARE(s.homepage, QStringLiteral("https://somafm.com/lush/"));
}

void TestRadioCatalog::rejectsInvalidJson() {
  QString error;
  const auto stations = Radio::Catalog::fromJson("{\"stations\":[", &error);
  QVERIFY(!error.isEmpty());
  QVERIFY(stations.isEmpty());
}

void TestRadioCatalog::rejectsNonObjectRoot() {
  QString error;
  Radio::Catalog::fromJson("[]", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsMissingStationsArray() {
  QString error;
  Radio::Catalog::fromJson("{\"version\":1}", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsStationWithoutId() {
  QString error;
  Radio::Catalog::fromJson(R"({"stations":[{"name":"A","url":"https://h/a"}]})", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsStationWithoutUrl() {
  QString error;
  Radio::Catalog::fromJson(R"({"stations":[{"id":"a","name":"A"}]})", &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::rejectsNonHttpUrl() {
  QString error;
  Radio::Catalog::fromJson(oneStation(R"("url":"ftp://h/a")"), &error);
  QVERIFY(!error.isEmpty());
}

void TestRadioCatalog::acceptsHttpUrl() {
  QString error;
  const auto stations = Radio::Catalog::fromJson(
    oneStation(R"("url":"http://relay4.slayradio.org:8000/","codec":"mp3","bitrate":128)"), &error);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QCOMPARE(stations.size(), 1);
}

void TestRadioCatalog::rejectsDuplicateIds() {
  QString error;
  Radio::Catalog::fromJson(QString(R"({"stations":[)"
                                   R"({"id":"a","name":"A",%1},)"
                                   R"({"id":"a","name":"B",%1}]})")
                             .arg(kUrl).toUtf8(), &error);
  QVERIFY(!error.isEmpty());
  QVERIFY(error.contains("a"));
}

void TestRadioCatalog::clearsErrorOnSuccess() {
  QString error = QStringLiteral("stale");
  Radio::Catalog::fromJson(oneStation(kUrl), &error);
  QVERIFY(error.isEmpty());
}

void TestRadioCatalog::groupsAreOrderedAndDeduped() {
  const auto json = QString(R"({"stations":[)"
                            R"({"id":"a","name":"A","group":"Zed",%1},)"
                            R"({"id":"b","name":"B","group":"Alpha",%1},)"
                            R"({"id":"c","name":"C","group":"Zed",%1}]})")
                      .arg(kUrl).toUtf8();
  const auto stations = Radio::Catalog::fromJson(json);
  QCOMPARE(Radio::Catalog::groups(stations), QStringList({"Zed", "Alpha"}));
}

void TestRadioCatalog::ungroupedStationsHaveNoGroups() {
  const auto stations = Radio::Catalog::fromJson(oneStation(kUrl));
  QVERIFY(stations.first().group.isEmpty());
  QVERIFY(Radio::Catalog::groups(stations).isEmpty());
}

void TestRadioCatalog::subtitleFormatsCodecAndBitrate() {
  Station s;
  s.codec = QStringLiteral("mp3");
  s.bitrate = 256;
  QCOMPARE(s.subtitle(), QStringLiteral("MP3 256k"));
}

void TestRadioCatalog::subtitleCodecOnlyWhenNoBitrate() {
  Station s;
  s.codec = QStringLiteral("aac");
  QCOMPARE(s.subtitle(), QStringLiteral("AAC"));
}

void TestRadioCatalog::subtitleEmptyWhenNoCodec() {
  Station s;
  s.bitrate = 128;
  QVERIFY(s.subtitle().isEmpty());
}

QTEST_MAIN(TestRadioCatalog)
#include "tst_radiocatalog.moc"
