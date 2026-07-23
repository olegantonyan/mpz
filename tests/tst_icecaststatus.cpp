#include <QtTest>

#include "radio/icecaststatus.h"

class TestIcecastStatus : public QObject {
  Q_OBJECT
private slots:
  void xIcyTitleWhenStandardFieldsNull();
  void standardTitleWins();
  void displayTitleBeforeXIcyTitle();
  void trackListLastEntryIsCurrent();
  void sourceArraySelectedByListenurl();
  void sourceArraySingleEntryTaken();
  void sourceArrayNoMatchIsEmpty();
  void missingSourceReportsNotFound();
  void malformedJsonIsEmpty();
  void emptyTrackListIsEmpty();
  void serverNameIsNeverATitle();
  void statusJsonUrlFromStreamUrl();
  void statusJsonUrlStripsCredentials();
};

namespace {
  // trimmed shape of premium.philosomatika.fm/status-json.xsl?mount=...
  const QByteArray philosomatika =
    "{\"icestats\":{\"server_id\":\"Icecast 2.5.0\",\"source\":{"
    "\"bitrate\":256,\"display-title\":null,\"genre\":\"Psytrance\","
    "\"listenurl\":\"http://premium.philosomatika.fm:8000/PhilosomatikaPremium256\","
    "\"server_name\":\"Philosomatika Premium (256KB)\",\"title\":null,"
    "\"metadata\":{\"x_icy_title\":\"Vertical Mode - Time Machine\"},"
    "\"playlist\":{\"trackList\":["
    "{\"title\":\"Somatic Cell - Mad Cow In A Cell\"},"
    "{\"title\":\"Alien Code, Eclipse Echoes - First Contact\"},"
    "{\"title\":\"Vertical Mode - Time Machine\"}]}}}}";
}

void TestIcecastStatus::xIcyTitleWhenStandardFieldsNull() {
  bool found = false;
  QCOMPARE(Radio::nowPlayingFromStatusJson(philosomatika,
                                           QStringLiteral("/PhilosomatikaPremium256"), &found),
           QStringLiteral("Vertical Mode - Time Machine"));
  QVERIFY(found);
}

void TestIcecastStatus::standardTitleWins() {
  const QByteArray body =
    "{\"icestats\":{\"source\":{\"title\":\"Standard Title\","
    "\"metadata\":{\"x_icy_title\":\"Legacy Title\"}}}}";
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/m")),
           QStringLiteral("Standard Title"));
}

void TestIcecastStatus::displayTitleBeforeXIcyTitle() {
  const QByteArray body =
    "{\"icestats\":{\"source\":{\"title\":null,\"display-title\":\"Shown\","
    "\"metadata\":{\"x_icy_title\":\"Legacy\"}}}}";
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/m")),
           QStringLiteral("Shown"));
}

void TestIcecastStatus::trackListLastEntryIsCurrent() {
  const QByteArray body =
    "{\"icestats\":{\"source\":{\"title\":null,\"display-title\":null,"
    "\"playlist\":{\"trackList\":[{\"title\":\"Oldest\"},{\"title\":\"Newest\"}]}}}}";
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/m")),
           QStringLiteral("Newest"));
}

void TestIcecastStatus::sourceArraySelectedByListenurl() {
  const QByteArray body =
    "{\"icestats\":{\"source\":["
    "{\"listenurl\":\"http://h:8000/one\",\"title\":\"First Stream\"},"
    "{\"listenurl\":\"http://h:8000/two\",\"title\":\"Second Stream\"}]}}";
  bool found = false;
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/two"), &found),
           QStringLiteral("Second Stream"));
  QVERIFY(found);
}

void TestIcecastStatus::sourceArraySingleEntryTaken() {
  const QByteArray body =
    "{\"icestats\":{\"source\":[{\"listenurl\":\"http://h:8000/other\",\"title\":\"Only\"}]}}";
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/mismatch")),
           QStringLiteral("Only"));
}

void TestIcecastStatus::sourceArrayNoMatchIsEmpty() {
  const QByteArray body =
    "{\"icestats\":{\"source\":["
    "{\"listenurl\":\"http://h:8000/one\",\"title\":\"A\"},"
    "{\"listenurl\":\"http://h:8000/two\",\"title\":\"B\"}]}}";
  bool found = true;
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/three"), &found), QString());
  QVERIFY(!found);
}

// the wrong-mount response: http 200, no source key at all
void TestIcecastStatus::missingSourceReportsNotFound() {
  const QByteArray body = "{\"icestats\":{\"server_id\":\"Icecast 2.5.0\",\"dummy\":null}}";
  bool found = true;
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/m"), &found), QString());
  QVERIFY(!found);
}

void TestIcecastStatus::malformedJsonIsEmpty() {
  bool found = true;
  QCOMPARE(Radio::nowPlayingFromStatusJson("not json at all", QStringLiteral("/m"), &found), QString());
  QVERIFY(!found);
  QCOMPARE(Radio::nowPlayingFromStatusJson(QByteArray(), QStringLiteral("/m")), QString());
}

void TestIcecastStatus::emptyTrackListIsEmpty() {
  const QByteArray body =
    "{\"icestats\":{\"source\":{\"title\":null,\"playlist\":{\"trackList\":[]}}}}";
  bool found = false;
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/m"), &found), QString());
  QVERIFY(found);
}

void TestIcecastStatus::serverNameIsNeverATitle() {
  const QByteArray body =
    "{\"icestats\":{\"source\":{\"server_name\":\"Some Station\",\"title\":null}}}";
  QCOMPARE(Radio::nowPlayingFromStatusJson(body, QStringLiteral("/m")), QString());
}

void TestIcecastStatus::statusJsonUrlFromStreamUrl() {
  const QUrl u = Radio::statusJsonUrl(QUrl(QStringLiteral("http://host:8000/mount")));
  QCOMPARE(u.scheme(), QStringLiteral("http"));
  QCOMPARE(u.host(), QStringLiteral("host"));
  QCOMPARE(u.port(), 8000);
  QCOMPARE(u.path(), QStringLiteral("/status-json.xsl"));
  QVERIFY(u.query(QUrl::FullyDecoded).contains(QStringLiteral("mount=/mount")));
}

void TestIcecastStatus::statusJsonUrlStripsCredentials() {
  const QUrl u = Radio::statusJsonUrl(QUrl(QStringLiteral("https://user:pass@host/m")));
  QVERIFY(u.userInfo().isEmpty());
  QCOMPARE(u.scheme(), QStringLiteral("https"));
  QVERIFY(!u.toString().contains(QStringLiteral("pass")));
}

QTEST_GUILESS_MAIN(TestIcecastStatus)
#include "tst_icecaststatus.moc"
