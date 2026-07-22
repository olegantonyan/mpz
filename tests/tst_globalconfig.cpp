#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "config/global.h"
#include "radio/station.h"

class TestGlobalConfig : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void stopWhenTrackRemoved_defaultsFalse();
  void stopWhenTrackRemoved_readsTrue();
  void stopWhenTrackRemoved_readsFalse();
  void disableGapless_defaultsFalse();
  void disableGapless_readsTrue();
  void gaplessCacheSizeMb_defaultsZero();
  void gaplessCacheSizeMb_roundTrips();
  void lyricsProviders_defaultsToEmpty();
  void lyricsProviders_ignoresMalformedValue();
  void lyricsProviders_keepsLegacyBuiltinEntries();
  void lyricsProviders_explicitEmptyStaysEmpty();
  void lyricsProviders_emptyListRoundTrips();
  void coverProviders_defaultsToEmpty();
  void coverProviders_readsOrder();
  void coverProviders_roundTrips();
  void radioStations_absentByDefault();
  void radioStations_roundTrips();
  void radioStations_emptyListRoundTrips();

private:
  QTemporaryDir tempDir;
  void writeGlobalYaml(const QByteArray &body);
};

void TestGlobalConfig::init() {
  tempDir.setAutoRemove(true);
  QVERIFY(tempDir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", tempDir.path().toUtf8());
}

void TestGlobalConfig::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestGlobalConfig::writeGlobalYaml(const QByteArray &body) {
  QFile f(tempDir.filePath(QStringLiteral("global.yml")));
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
  f.write(body);
}

void TestGlobalConfig::stopWhenTrackRemoved_defaultsFalse() {
  Config::Global g;
  QCOMPARE(g.stopWhenTrackRemoved(), false);
}

void TestGlobalConfig::stopWhenTrackRemoved_readsTrue() {
  writeGlobalYaml("stop_when_track_removed: true\n");
  Config::Global g;
  QCOMPARE(g.stopWhenTrackRemoved(), true);
}

void TestGlobalConfig::stopWhenTrackRemoved_readsFalse() {
  writeGlobalYaml("stop_when_track_removed: false\n");
  Config::Global g;
  QCOMPARE(g.stopWhenTrackRemoved(), false);
}

// Absent key means gapless is on (the stored flag is the inverted "disabled").
void TestGlobalConfig::disableGapless_defaultsFalse() {
  Config::Global g;
  QCOMPARE(g.disableGapless(), false);
}

void TestGlobalConfig::disableGapless_readsTrue() {
  writeGlobalYaml("disable_gapless: true\n");
  Config::Global g;
  QCOMPARE(g.disableGapless(), true);
}

// Absent key reads 0, which call sites treat as "use the default".
void TestGlobalConfig::gaplessCacheSizeMb_defaultsZero() {
  Config::Global g;
  QCOMPARE(g.gaplessCacheSizeMb(), 0);
}

void TestGlobalConfig::gaplessCacheSizeMb_roundTrips() {
  {
    Config::Global g;
    g.saveGaplessCacheSizeMb(250);
    g.sync();
  }
  Config::Global reloaded;
  QCOMPARE(reloaded.gaplessCacheSizeMb(), 250);
}

// No online provider is enabled until the user picks one; built-in lyrics
// sources are always on and never appear in the config.
void TestGlobalConfig::lyricsProviders_defaultsToEmpty() {
  Config::Global g;
  QVERIFY(g.lyricsProviders().isEmpty());
}

void TestGlobalConfig::lyricsProviders_ignoresMalformedValue() {
  writeGlobalYaml("lyrics: nonsense\n");
  Config::Global g;
  QVERIFY(g.lyricsProviders().isEmpty());
}

// Configs written before built-in sources became always-on still name them.
// They are returned verbatim here and dropped by ProviderChain::filterKnown.
void TestGlobalConfig::lyricsProviders_keepsLegacyBuiltinEntries() {
  writeGlobalYaml("lyrics:\n  providers:\n    - embedded\n    - sidecar\n    - lrclib\n");
  Config::Global g;
  const QStringList expected{ QStringLiteral("embedded"), QStringLiteral("sidecar"), QStringLiteral("lrclib") };
  QCOMPARE(g.lyricsProviders(), expected);
}

// An explicitly empty list means the user turned everything off. It must not
// fall back to the defaults.
void TestGlobalConfig::lyricsProviders_explicitEmptyStaysEmpty() {
  writeGlobalYaml("lyrics:\n  providers: []\n");
  Config::Global g;
  QVERIFY(g.lyricsProviders().isEmpty());
}

void TestGlobalConfig::lyricsProviders_emptyListRoundTrips() {
  {
    Config::Global g;
    QVERIFY(g.saveLyricsProviders({}));
    g.sync();
  }
  Config::Global reloaded;
  QVERIFY(reloaded.lyricsProviders().isEmpty());
}

void TestGlobalConfig::coverProviders_defaultsToEmpty() {
  Config::Global g;
  QVERIFY(g.coverProviders().isEmpty());
}

void TestGlobalConfig::coverProviders_readsOrder() {
  writeGlobalYaml("covers:\n  providers:\n    - itunes\n    - deezer\n");
  Config::Global g;
  const QStringList expected{ QStringLiteral("itunes"), QStringLiteral("deezer") };
  QCOMPARE(g.coverProviders(), expected);
}

void TestGlobalConfig::coverProviders_roundTrips() {
  const QStringList saved{ QStringLiteral("deezer"), QStringLiteral("coverartarchive") };
  {
    Config::Global g;
    QVERIFY(g.saveCoverProviders(saved));
    g.sync();
  }
  Config::Global reloaded;
  QCOMPARE(reloaded.coverProviders(), saved);
}

void TestGlobalConfig::radioStations_absentByDefault() {
  Config::Global g;
  QVERIFY(g.radioStations().isEmpty());
}

void TestGlobalConfig::radioStations_roundTrips() {
  Radio::Station a;
  a.id = "somafm-groovesalad";
  a.name = "Groove Salad";
  a.group = "SomaFM";
  a.url = "https://ice.somafm.com/groovesalad-256-mp3";
  a.codec = "mp3";
  a.bitrate = 256;
  a.homepage = "https://somafm.com/groovesalad/";
  Radio::Station b;
  b.id = "slay";
  b.name = "SLAY Radio";
  b.url = "http://relay4.slayradio.org:8300/";
  b.codec = "aac";
  b.bitrate = 128;

  {
    Config::Global g;
    QVERIFY(g.saveRadioStations({a, b}));
    g.sync();
  }

  Config::Global reloaded;
  const auto stations = reloaded.radioStations();
  QCOMPARE(stations.size(), 2);
  QCOMPARE(stations.at(0).id, a.id);
  QCOMPARE(stations.at(0).name, a.name);
  QCOMPARE(stations.at(0).group, a.group);
  QCOMPARE(stations.at(0).url, a.url);
  QCOMPARE(stations.at(0).codec, a.codec);
  QCOMPARE(stations.at(0).bitrate, a.bitrate);
  QCOMPARE(stations.at(0).homepage, a.homepage);
  QCOMPARE(stations.at(1).id, b.id);
  QCOMPARE(stations.at(1).url, b.url);
  QCOMPARE(stations.at(1).bitrate, b.bitrate);
}

void TestGlobalConfig::radioStations_emptyListRoundTrips() {
  {
    Config::Global g;
    QVERIFY(g.saveRadioStations({}));
    g.sync();
  }
  Config::Global reloaded;
  QVERIFY(reloaded.radioStations().isEmpty());
}

QTEST_GUILESS_MAIN(TestGlobalConfig)
#include "tst_globalconfig.moc"
