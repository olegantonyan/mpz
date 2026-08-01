#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "config/local.h"
#include "playlist/playlist.h"
#include "track.h"

class TestLocalConfig : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void playlists_absentByDefault();
  void playlists_roundTripAlbumArtistAndDiscNumber();
  void playlists_absentNewKeysReadAsEmpty();
  void playlists_unknownKeysDoNotBreakLoading();
  void serialize_omitsEmptyAlbumArtistAndDiscNumber();

private:
  QTemporaryDir tempDir;
  void writeLocalYaml(const QByteArray &body);
  QByteArray readLocalYaml() const;
  static QList<std::shared_ptr<Playlist::Playlist>> onePlaylist(const Track &t);
};

void TestLocalConfig::init() {
  tempDir.setAutoRemove(true);
  QVERIFY(tempDir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", tempDir.path().toUtf8());
}

void TestLocalConfig::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestLocalConfig::writeLocalYaml(const QByteArray &body) {
  QFile f(tempDir.filePath(QStringLiteral("local.yml")));
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
  f.write(body);
}

QByteArray TestLocalConfig::readLocalYaml() const {
  QFile f(tempDir.filePath(QStringLiteral("local.yml")));
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return QByteArray();
  }
  return f.readAll();
}

QList<std::shared_ptr<Playlist::Playlist>> TestLocalConfig::onePlaylist(const Track &t) {
  std::shared_ptr<Playlist::Playlist> pl(new Playlist::Playlist());
  pl->rename(QStringLiteral("pl"));
  pl->load(QVector<Track>{ t });
  return QList<std::shared_ptr<Playlist::Playlist>>{ pl };
}

void TestLocalConfig::playlists_absentByDefault() {
  Config::Local l;
  QVERIFY(l.playlists().isEmpty());
}

void TestLocalConfig::playlists_roundTripAlbumArtistAndDiscNumber() {
  Track t(QStringLiteral("/music/song.flac"), 0,
          QStringLiteral("Artist"), QStringLiteral("Album"), QStringLiteral("Title"),
          3, 2024, 180000, 2, 320, 44100);
  t.setAlbumArtist(QStringLiteral("Various Artists"));
  t.setDiscNumber(QStringLiteral("2/3"));

  {
    Config::Local l;
    auto list = onePlaylist(t);
    QVERIFY(l.savePlaylists(list));
    QVERIFY(l.sync());
  }

  Config::Local reloaded;
  const auto playlists = reloaded.playlists();
  QCOMPARE(playlists.size(), 1);
  const auto tracks = playlists.first()->tracks();
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.first().album_artist(), QStringLiteral("Various Artists"));
  QCOMPARE(tracks.first().disc_number(),  QStringLiteral("2/3"));
}

void TestLocalConfig::playlists_absentNewKeysReadAsEmpty() {
  writeLocalYaml(
    "__app_version__: 2.1.2\n"
    "playlists:\n"
    "  - name: pl\n"
    "    playback_order_override: 0\n"
    "    tracks:\n"
    "      - path: /music/song.flac\n"
    "        begin: 0\n"
    "        cue: false\n"
    "        artist: Artist\n"
    "        album: Album\n"
    "        title: Title\n"
    "        track_number: 3\n"
    "        year: 2024\n"
    "        duration: 180000\n"
    "        channels: 2\n"
    "        bitrate: 320\n"
    "        samplerate: 44100\n");

  Config::Local l;
  const auto playlists = l.playlists();
  QCOMPARE(playlists.size(), 1);
  const auto tracks = playlists.first()->tracks();
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.first().artist(), QStringLiteral("Artist"));
  QVERIFY(tracks.first().album_artist().isEmpty());
  QVERIFY(tracks.first().disc_number().isEmpty());
}

void TestLocalConfig::playlists_unknownKeysDoNotBreakLoading() {
  writeLocalYaml(
    "__app_version__: 99.0.0\n"
    "playlists:\n"
    "  - name: pl\n"
    "    playback_order_override: 0\n"
    "    tracks:\n"
    "      - path: /music/song.flac\n"
    "        begin: 0\n"
    "        cue: false\n"
    "        artist: Artist\n"
    "        album: Album\n"
    "        title: Title\n"
    "        track_number: 3\n"
    "        year: 2024\n"
    "        duration: 180000\n"
    "        channels: 2\n"
    "        bitrate: 320\n"
    "        samplerate: 44100\n"
    "        album_artist: Various Artists\n"
    "        disc_number: 2/3\n"
    "        some_future_tag: whatever\n");

  Config::Local l;
  const auto playlists = l.playlists();
  QCOMPARE(playlists.size(), 1);
  const auto tracks = playlists.first()->tracks();
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.first().album_artist(), QStringLiteral("Various Artists"));
  QCOMPARE(tracks.first().disc_number(),  QStringLiteral("2/3"));
}

void TestLocalConfig::serialize_omitsEmptyAlbumArtistAndDiscNumber() {
  Track t(QStringLiteral("/music/song.flac"), 0,
          QStringLiteral("Artist"), QStringLiteral("Album"), QStringLiteral("Title"),
          3, 2024, 180000, 2, 320, 44100);
  {
    Config::Local l;
    auto list = onePlaylist(t);
    QVERIFY(l.savePlaylists(list));
    QVERIFY(l.sync());
  }

  const QByteArray yaml = readLocalYaml();
  QVERIFY(!yaml.isEmpty());
  QVERIFY(yaml.contains("track_number"));
  QVERIFY(!yaml.contains("album_artist"));
  QVERIFY(!yaml.contains("disc_number"));
}

QTEST_GUILESS_MAIN(TestLocalConfig)
#include "tst_localconfig.moc"
