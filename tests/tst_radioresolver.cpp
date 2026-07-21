#include <QtTest>

#include "radio/resolver.h"

class TestRadioResolver : public QObject {
  Q_OBJECT
private slots:
  void plsFirstFile();
  void plsSkipsNonHttp();
  void m3uFirstHttpLine();
  void m3uSkipsComments();
  void emptyBody();
  void noStreamLine();
  void looksLikePlaylistByExtension();
  void rawUrlIsNotPlaylist();
  void guessCodecAndBitrateFromUrl();
  void guessLeavesUnfilledWhenNoHint();
  void guessKeepsExistingWhenNoHint();
};

void TestRadioResolver::plsFirstFile() {
  const QByteArray body =
    "[playlist]\n"
    "File1=https://ice5.somafm.com/groovesalad-256-mp3\n"
    "File2=https://ice3.somafm.com/groovesalad-256-mp3\n";
  QCOMPARE(Radio::firstStreamUrl(body),
           QStringLiteral("https://ice5.somafm.com/groovesalad-256-mp3"));
}

void TestRadioResolver::plsSkipsNonHttp() {
  const QByteArray body = "[playlist]\nFile1=/local/thing.mp3\nFile2=http://h/stream\n";
  QCOMPARE(Radio::firstStreamUrl(body), QStringLiteral("http://h/stream"));
}

void TestRadioResolver::m3uFirstHttpLine() {
  const QByteArray body = "#EXTM3U\n#EXTINF:-1,SLAY Radio\nhttp://relay4.slayradio.org:8300/\n";
  QCOMPARE(Radio::firstStreamUrl(body), QStringLiteral("http://relay4.slayradio.org:8300/"));
}

void TestRadioResolver::m3uSkipsComments() {
  const QByteArray body = "# a comment\n\nhttps://h/first\nhttps://h/second\n";
  QCOMPARE(Radio::firstStreamUrl(body), QStringLiteral("https://h/first"));
}

void TestRadioResolver::emptyBody() {
  QVERIFY(Radio::firstStreamUrl(QByteArray()).isEmpty());
}

void TestRadioResolver::noStreamLine() {
  QVERIFY(Radio::firstStreamUrl("[playlist]\nNumberOfEntries=0\n").isEmpty());
}

void TestRadioResolver::looksLikePlaylistByExtension() {
  QVERIFY(Radio::looksLikePlaylist("https://somafm.com/groovesalad.pls"));
  QVERIFY(Radio::looksLikePlaylist("/home/me/stations.m3u"));
  QVERIFY(Radio::looksLikePlaylist("https://x/y.m3u8"));
}

void TestRadioResolver::rawUrlIsNotPlaylist() {
  QVERIFY(!Radio::looksLikePlaylist("https://ice.somafm.com/groovesalad-256-mp3"));
  QVERIFY(!Radio::looksLikePlaylist("http://relay4.slayradio.org:8300/"));
}

void TestRadioResolver::guessCodecAndBitrateFromUrl() {
  QString codec;
  quint16 bitrate = 0;
  Radio::guessStreamFormat("https://ice.somafm.com/groovesalad-256-mp3", &codec, &bitrate);
  QCOMPARE(codec, QStringLiteral("mp3"));
  QCOMPARE(bitrate, quint16{256});

  codec.clear();
  bitrate = 0;
  Radio::guessStreamFormat("https://stream.radioparadise.com/aac-320", &codec, &bitrate);
  QCOMPARE(codec, QStringLiteral("aac"));
  QCOMPARE(bitrate, quint16{320});

  codec.clear();
  bitrate = 0;
  Radio::guessStreamFormat("https://kexp.streamguys1.com/kexp160.aac", &codec, &bitrate);
  QCOMPARE(codec, QStringLiteral("aac"));
  QCOMPARE(bitrate, quint16{160});
}

void TestRadioResolver::guessLeavesUnfilledWhenNoHint() {
  QString codec;
  quint16 bitrate = 0;
  Radio::guessStreamFormat("http://relay4.slayradio.org:8300/", &codec, &bitrate);
  QVERIFY(codec.isEmpty());
  QCOMPARE(bitrate, quint16{0});
}

void TestRadioResolver::guessKeepsExistingWhenNoHint() {
  QString codec = QStringLiteral("mp3");
  quint16 bitrate = 128;
  Radio::guessStreamFormat("http://relay4.slayradio.org:8300/", &codec, &bitrate);
  QCOMPARE(codec, QStringLiteral("mp3"));
  QCOMPARE(bitrate, quint16{128});
}

QTEST_MAIN(TestRadioResolver)
#include "tst_radioresolver.moc"
