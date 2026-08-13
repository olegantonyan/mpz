#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "config/global.h"
#include "replaygain/manager.h"

class TestReplayGainManager : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();

  void defaultsAreOffAndSidecar();
  void settingsRoundTripThroughConfig();
  void planGroupsByFolder();
  void planSkipsStreamsAndMissingFiles();
  void planWantsAlbumGainOnlyWhenFolderIsCovered();
  void planSkipsAlreadyScannedTracksWithoutAlbumGain();
  void planRescansWholeFolderWhenAlbumGainIsMissing();
  void forceRescansEverything();
  void planMergesCueSlicesIntoOneFile();
  void planSkipsAlbumGainForPartialCue();
  void planSkipsAlbumGainForAnOversizedFolder();
  void planMarksTagWritingFromStorageMode();
  void tagsModeIgnoresTheSidecarWhenDecidingWhatToScan();
  void appliedGainTextNamesTheModeValueAndSource();

private:
  QString makeTrackFile(const QString &relative);
  static ReplayGain::Gain trackGain(double db);
  static ReplayGain::Gain albumGain(double db);

  std::unique_ptr<QTemporaryDir> dir;
};

void TestReplayGainManager::init() {
  dir = std::make_unique<QTemporaryDir>();
  QVERIFY(dir->isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir->filePath(QStringLiteral("config")).toUtf8());
}

void TestReplayGainManager::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

QString TestReplayGainManager::makeTrackFile(const QString &relative) {
  const QString path = dir->filePath(relative);
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly)) {
    return QString();
  }
  f.write("audio");
  f.close();
  return path;
}

ReplayGain::Gain TestReplayGainManager::trackGain(double db) {
  ReplayGain::Gain g;
  g.track_db = db;
  g.track_peak = 0.5;
  g.has_track = true;
  return g;
}

ReplayGain::Gain TestReplayGainManager::albumGain(double db) {
  ReplayGain::Gain g = trackGain(db);
  g.album_db = db;
  g.album_peak = 0.5;
  g.has_album = true;
  return g;
}

void TestReplayGainManager::defaultsAreOffAndSidecar() {
  Config::Global global;
  ReplayGain::Manager m(global);

  QCOMPARE(m.settings().mode, ReplayGain::Mode::Off);
  QCOMPARE(m.settings().storage, ReplayGain::StorageMode::Sidecar);
  QVERIFY(m.settings().prevent_clipping);
  QCOMPARE(m.settings().preamp_db, 0.0);
  QCOMPARE(m.settings().fallback_db, 0.0);
}

void TestReplayGainManager::settingsRoundTripThroughConfig() {
  ReplayGain::Settings s;
  s.mode = ReplayGain::Mode::Album;
  s.storage = ReplayGain::StorageMode::Tags;
  s.preamp_db = -3.5;
  s.fallback_db = 2.5;
  s.prevent_clipping = false;

  {
    Config::Global global;
    ReplayGain::Manager m(global);
    m.setSettings(s);
    global.sync();
  }

  Config::Global global;
  ReplayGain::Manager m(global);
  QCOMPARE(m.settings().mode, ReplayGain::Mode::Album);
  QCOMPARE(m.settings().storage, ReplayGain::StorageMode::Tags);
  QCOMPARE(m.settings().preamp_db, -3.5);
  QCOMPARE(m.settings().fallback_db, 2.5);
  QVERIFY(!m.settings().prevent_clipping);
}

void TestReplayGainManager::planGroupsByFolder() {
  const QString a1 = makeTrackFile(QStringLiteral("albumA/01.flac"));
  const QString a2 = makeTrackFile(QStringLiteral("albumA/02.flac"));
  const QString b1 = makeTrackFile(QStringLiteral("albumB/01.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);

  const auto jobs = m.planScan({Track(a1), Track(a2), Track(b1)}, false);
  QCOMPARE(jobs.size(), 2);
  QCOMPARE(jobs.at(0).files.size(), 2);
  QCOMPARE(jobs.at(1).files.size(), 1);
  QVERIFY(jobs.at(0).want_album);
  QCOMPARE(jobs.at(0).sliceCount(), 2);
}

void TestReplayGainManager::planSkipsStreamsAndMissingFiles() {
  const QString real = makeTrackFile(QStringLiteral("albumA/01.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);

  const Track stream(QUrl(QStringLiteral("http://example.com/live")), QStringLiteral("live"));
  const Track missing(dir->filePath(QStringLiteral("albumA/gone.flac")));

  const auto jobs = m.planScan({Track(real), stream, missing}, false);
  QCOMPARE(jobs.size(), 1);
  QCOMPARE(jobs.at(0).sliceCount(), 1);
  QCOMPARE(jobs.at(0).files.at(0).path, real);
}

void TestReplayGainManager::planWantsAlbumGainOnlyWhenFolderIsCovered() {
  const QString a1 = makeTrackFile(QStringLiteral("albumA/01.flac"));
  const QString a2 = makeTrackFile(QStringLiteral("albumA/02.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);

  const auto partial = m.planScan({Track(a1)}, false);
  QCOMPARE(partial.size(), 1);
  QVERIFY(!partial.at(0).want_album);

  const auto whole = m.planScan({Track(a1), Track(a2)}, false);
  QCOMPARE(whole.size(), 1);
  QVERIFY(whole.at(0).want_album);
}

void TestReplayGainManager::planSkipsAlreadyScannedTracksWithoutAlbumGain() {
  const QString scanned = makeTrackFile(QStringLiteral("albumA/01.flac"));
  const QString fresh = makeTrackFile(QStringLiteral("albumA/02.flac"));
  makeTrackFile(QStringLiteral("albumA/03.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);
  QVERIFY(m.store().put(scanned, 0, trackGain(-6.0)));

  const auto jobs = m.planScan({Track(scanned), Track(fresh)}, false);
  QCOMPARE(jobs.size(), 1);
  QVERIFY(!jobs.at(0).want_album);
  QCOMPARE(jobs.at(0).sliceCount(), 1);
  QCOMPARE(jobs.at(0).files.at(0).path, fresh);
}

void TestReplayGainManager::planRescansWholeFolderWhenAlbumGainIsMissing() {
  const QString scanned = makeTrackFile(QStringLiteral("albumA/01.flac"));
  const QString fresh = makeTrackFile(QStringLiteral("albumA/02.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);
  QVERIFY(m.store().put(scanned, 0, trackGain(-6.0)));

  const auto jobs = m.planScan({Track(scanned), Track(fresh)}, false);
  QCOMPARE(jobs.size(), 1);
  QVERIFY(jobs.at(0).want_album);
  QCOMPARE(jobs.at(0).sliceCount(), 2);

  QVERIFY(m.store().put(scanned, 0, albumGain(-6.0)));
  QVERIFY(m.store().put(fresh, 0, albumGain(-6.0)));
  QVERIFY(m.planScan({Track(scanned), Track(fresh)}, false).isEmpty());
}

void TestReplayGainManager::forceRescansEverything() {
  const QString path = makeTrackFile(QStringLiteral("albumA/01.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);
  QVERIFY(m.store().put(path, 0, albumGain(-6.0)));

  QVERIFY(m.planScan({Track(path)}, false).isEmpty());
  QCOMPARE(m.planScan({Track(path)}, true).size(), 1);
}

void TestReplayGainManager::planMergesCueSlicesIntoOneFile() {
  const QString container = makeTrackFile(QStringLiteral("albumA/whole.flac"));

  Track second(container, 180000);
  second.setCue();
  second.setDuration(120000);
  Track first(container, 0);
  first.setCue();
  first.setDuration(180000);

  Config::Global global;
  ReplayGain::Manager m(global);

  const auto jobs = m.planScan({second, first}, false);
  QCOMPARE(jobs.size(), 1);
  QCOMPARE(jobs.at(0).files.size(), 1);
  QVERIFY(jobs.at(0).want_album);
  QCOMPARE(jobs.at(0).files.at(0).slices.size(), 2);
  QCOMPARE(jobs.at(0).files.at(0).slices.at(0).begin_ms, 0ULL);
  QCOMPARE(jobs.at(0).files.at(0).slices.at(0).duration_ms, 180000ULL);
  QCOMPARE(jobs.at(0).files.at(0).slices.at(1).begin_ms, 180000ULL);
  QCOMPARE(jobs.at(0).files.at(0).slices.at(1).duration_ms, 120000ULL);
}

void TestReplayGainManager::planSkipsAlbumGainForPartialCue() {
  const QString container = makeTrackFile(QStringLiteral("albumA/whole.flac"));

  Track second(container, 180000);
  second.setCue();
  second.setDuration(120000);

  Config::Global global;
  ReplayGain::Manager m(global);

  const auto jobs = m.planScan({second}, false);
  QCOMPARE(jobs.size(), 1);
  QVERIFY(!jobs.at(0).want_album);
}

void TestReplayGainManager::planSkipsAlbumGainForAnOversizedFolder() {
  QVector<Track> tracks;
  for (int i = 0; i < 70; i++) {
    tracks << Track(makeTrackFile(QString("dump/%1.flac").arg(i, 3, 10, QChar('0'))));
  }

  Config::Global global;
  ReplayGain::Manager m(global);

  const auto jobs = m.planScan(tracks, false);
  QCOMPARE(jobs.size(), 1);
  QCOMPARE(jobs.at(0).files.size(), 70);
  QVERIFY(!jobs.at(0).want_album);
}

void TestReplayGainManager::planMarksTagWritingFromStorageMode() {
  const QString path = makeTrackFile(QStringLiteral("albumA/01.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);

  QVERIFY(!m.planScan({Track(path)}, false).at(0).write_tags);

  ReplayGain::Settings s = m.settings();
  s.storage = ReplayGain::StorageMode::Tags;
  m.setSettings(s);

  QVERIFY(m.planScan({Track(path)}, false).at(0).write_tags);
}

void TestReplayGainManager::tagsModeIgnoresTheSidecarWhenDecidingWhatToScan() {
  const QString path = makeTrackFile(QStringLiteral("albumA/01.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);
  QVERIFY(m.store().put(path, 0, albumGain(-6.0)));
  QVERIFY(m.planScan({Track(path)}, false).isEmpty());

  ReplayGain::Settings s = m.settings();
  s.storage = ReplayGain::StorageMode::Tags;
  m.setSettings(s);

  // the file carries no tags, so tags mode has nothing analysed yet
  const auto jobs = m.planScan({Track(path)}, false);
  QCOMPARE(jobs.size(), 1);
  QVERIFY(jobs.at(0).write_tags);
}

void TestReplayGainManager::appliedGainTextNamesTheModeValueAndSource() {
  const QString scanned = makeTrackFile(QStringLiteral("albumA/01.flac"));
  const QString bare = makeTrackFile(QStringLiteral("albumA/02.flac"));

  Config::Global global;
  ReplayGain::Manager m(global);
  QVERIFY(m.store().put(scanned, 0, albumGain(-6.0)));

  QVERIFY(m.appliedGainText(Track(scanned)).isEmpty());

  ReplayGain::Settings s = m.settings();
  s.mode = ReplayGain::Mode::Track;
  m.setSettings(s);
  QCOMPARE(m.appliedGainText(Track(scanned)),
           QStringLiteral("ReplayGain: track -6.00 dB (sidecar)"));
  QCOMPARE(m.appliedGainText(Track(bare)), QStringLiteral("ReplayGain: fallback 0.00 dB (none)"));

  s.mode = ReplayGain::Mode::Album;
  s.preamp_db = 1.5;
  m.setSettings(s);
  QCOMPARE(m.appliedGainText(Track(scanned)),
           QStringLiteral("ReplayGain: album -4.50 dB (sidecar)"));

  const Track stream(QUrl(QStringLiteral("http://example.com/live")), QStringLiteral("live"));
  QVERIFY(m.appliedGainText(stream).isEmpty());
}

QTEST_MAIN(TestReplayGainManager)
#include "tst_replaygainmanager.moc"
