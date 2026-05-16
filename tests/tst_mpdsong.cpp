#include <QtTest>

#include "mpd_client/song.h"

using MpdClient::Song;

class TestMpdSong : public QObject {
  Q_OBJECT
private slots:
  void defaultsAreSentinels();
  void publicFieldsAreSettable();
  void tagsMapStartsEmpty();
};

void TestMpdSong::defaultsAreSentinels() {
  Song s;
  QVERIFY(s.title.isEmpty());
  QVERIFY(s.artist.isEmpty());
  QVERIFY(s.album.isEmpty());
  QVERIFY(s.albumArtist.isEmpty());
  QVERIFY(s.composer.isEmpty());
  QVERIFY(s.performer.isEmpty());
  QVERIFY(s.genre.isEmpty());
  QVERIFY(s.date.isEmpty());
  QVERIFY(s.name.isEmpty());
  QCOMPARE(s.trackNumber, -1);
  QCOMPARE(s.discNumber,  -1);
  QCOMPARE(s.duration,    -1);
  QCOMPARE(s.id,          -1);
  QCOMPARE(s.pos,         -1);
  QCOMPARE(s.prio,        0);
  QVERIFY(s.filepath.isEmpty());
  QVERIFY(s.musicBrainzArtistId.isEmpty());
  QVERIFY(s.musicBrainzAlbumId.isEmpty());
  QVERIFY(s.musicBrainzAlbumArtistId.isEmpty());
  QVERIFY(s.musicBrainzTrackId.isEmpty());
}

void TestMpdSong::publicFieldsAreSettable() {
  Song s;
  s.title  = QStringLiteral("T");
  s.artist = QStringLiteral("A");
  s.album  = QStringLiteral("L");
  s.duration    = 300;
  s.trackNumber = 5;
  s.filepath    = QStringLiteral("music/file.flac");
  QCOMPARE(s.title,       QStringLiteral("T"));
  QCOMPARE(s.artist,      QStringLiteral("A"));
  QCOMPARE(s.album,       QStringLiteral("L"));
  QCOMPARE(s.duration,    300);
  QCOMPARE(s.trackNumber, 5);
  QCOMPARE(s.filepath,    QStringLiteral("music/file.flac"));
}

void TestMpdSong::tagsMapStartsEmpty() {
  Song s;
  QVERIFY(s.tags.isEmpty());
}

QTEST_GUILESS_MAIN(TestMpdSong)
#include "tst_mpdsong.moc"
