#include <QtTest>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "replaygain/store.h"

#include <sqlite3.h>

class TestReplayGainStore : public QObject {
  Q_OBJECT
private slots:
  void init();

  void roundTrip();
  void absentEntryIsInvalid();
  void lastWriteWins();
  void awkwardPathsSurvive();
  void staleWhenSizeChanged();
  void staleWhenMtimeChanged();
  void albumOnlyEntry();
  void countTracksInsertsNotRewrites();
  void batchedWritesAreDurable();
  void pruneDropsEntriesForDeletedFiles();
  void pruneKeepsEntriesWhenTheFolderIsGone();
  void pruneIgnoresFoldersItWasNotGiven();
  void corruptFileIsQuarantinedNotFatal();
  void newerSchemaIsLeftAlone();

private:
  QString makeTrack(const QString &name, const QByteArray &content = "audio");
  QString storeDir();
  static ReplayGain::Gain sampleGain();

  std::unique_ptr<QTemporaryDir> dir;
};

void TestReplayGainStore::init() {
  dir = std::make_unique<QTemporaryDir>();
  dir->setAutoRemove(true);
  QVERIFY(dir->isValid());
}

QString TestReplayGainStore::storeDir() {
  const QString path = dir->filePath(QStringLiteral("db"));
  QDir().mkpath(path);
  return path;
}

QString TestReplayGainStore::makeTrack(const QString &name, const QByteArray &content) {
  const QString path = dir->filePath(name);
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly)) {
    return QString();
  }
  f.write(content);
  f.close();
  return path;
}

ReplayGain::Gain TestReplayGainStore::sampleGain() {
  ReplayGain::Gain g;
  g.track_db = -5.57;
  g.track_peak = 0.910858;
  g.album_db = -2.48;
  g.album_peak = 0.977203;
  g.has_track = true;
  g.has_album = true;
  return g;
}

void TestReplayGainStore::roundTrip() {
  const QString track = makeTrack(QStringLiteral("a.flac"));
  const QString db = storeDir();
  {
    ReplayGain::Store s(db);
    QVERIFY(s.isOpen());
    QVERIFY(s.put(track, 0, sampleGain()));
    QCOMPARE(s.count(), qint64(1));
  }

  ReplayGain::Store reopened(db);
  QCOMPARE(reopened.count(), qint64(1));
  const ReplayGain::Gain g = reopened.get(track, 0);
  QVERIFY(g.isValid());
  QCOMPARE(g.track_db, -5.57);
  QCOMPARE(g.track_peak, 0.910858);
  QCOMPARE(g.album_db, -2.48);
  QCOMPARE(g.album_peak, 0.977203);
}

void TestReplayGainStore::absentEntryIsInvalid() {
  const QString present = makeTrack(QStringLiteral("a.flac"));
  ReplayGain::Store s(storeDir());
  QVERIFY(!s.get(present, 0).isValid());
  QVERIFY(!s.get(dir->filePath(QStringLiteral("nope.flac")), 0).isValid());
}

void TestReplayGainStore::lastWriteWins() {
  const QString track = makeTrack(QStringLiteral("a.flac"));
  const QString db = storeDir();
  {
    ReplayGain::Store s(db);
    ReplayGain::Gain first = sampleGain();
    QVERIFY(s.put(track, 0, first));
    ReplayGain::Gain second = sampleGain();
    second.track_db = -9.99;
    QVERIFY(s.put(track, 0, second));
    QCOMPARE(s.count(), qint64(1));
  }
  ReplayGain::Store reopened(db);
  QCOMPARE(reopened.count(), qint64(1));
  QCOMPARE(reopened.get(track, 0).track_db, -9.99);
}

// The text format needed hand-rolled escaping for these; bound parameters do not.
void TestReplayGainStore::awkwardPathsSurvive() {
  const QString track = makeTrack(QStringLiteral("od\td\nname 'quoted' %pct_ ünïcode.flac"));
  QVERIFY(!track.isEmpty());
  const QString db = storeDir();
  {
    ReplayGain::Store s(db);
    QVERIFY(s.put(track, 0, sampleGain()));
  }
  ReplayGain::Store reopened(db);
  QVERIFY(reopened.get(track, 0).isValid());
}

void TestReplayGainStore::staleWhenSizeChanged() {
  const QString track = makeTrack(QStringLiteral("a.flac"));
  ReplayGain::Store s(storeDir());
  QVERIFY(s.put(track, 0, sampleGain()));
  QVERIFY(s.get(track, 0).isValid());

  QFile f(track);
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write("audio and then some");
  f.close();

  QCOMPARE(s.count(), qint64(1));
  QVERIFY(!s.get(track, 0).isValid());
}

void TestReplayGainStore::staleWhenMtimeChanged() {
  const QString track = makeTrack(QStringLiteral("a.flac"));
  ReplayGain::Store s(storeDir());
  QVERIFY(s.put(track, 0, sampleGain()));
  QVERIFY(s.get(track, 0).isValid());

  QFile f(track);
  QVERIFY(f.open(QIODevice::ReadWrite));
  QVERIFY(f.setFileTime(QDateTime::currentDateTime().addSecs(-4242), QFileDevice::FileModificationTime));
  f.close();

  QVERIFY(!s.get(track, 0).isValid());
}

void TestReplayGainStore::albumOnlyEntry() {
  const QString track = makeTrack(QStringLiteral("a.flac"));
  const QString db = storeDir();
  ReplayGain::Gain album;
  album.album_db = -2.48;
  album.album_peak = 0.977203;
  album.has_album = true;
  {
    ReplayGain::Store s(db);
    QVERIFY(s.put(track, 0, album));
  }
  ReplayGain::Store reopened(db);
  const ReplayGain::Gain g = reopened.get(track, 0);
  QVERIFY(g.isValid());
  QVERIFY(!g.has_track);
  QVERIFY(g.has_album);
  QCOMPARE(g.album_db, -2.48);
}

// count() is maintained by triggers, so an upsert must not be counted as a new row.
void TestReplayGainStore::countTracksInsertsNotRewrites() {
  const QString a = makeTrack(QStringLiteral("a.flac"));
  const QString b = makeTrack(QStringLiteral("b.flac"));
  ReplayGain::Store s(storeDir());
  QCOMPARE(s.count(), qint64(0));
  QVERIFY(s.put(a, 0, sampleGain()));
  QCOMPARE(s.count(), qint64(1));
  QVERIFY(s.put(a, 0, sampleGain()));
  QCOMPARE(s.count(), qint64(1));
  QVERIFY(s.put(a, 5000, sampleGain())); // a second slice of the same file is its own row
  QCOMPARE(s.count(), qint64(2));
  QVERIFY(s.put(b, 0, sampleGain()));
  QCOMPARE(s.count(), qint64(3));
}

void TestReplayGainStore::batchedWritesAreDurable() {
  const QString db = storeDir();
  QStringList tracks;
  {
    ReplayGain::Store s(db);
    s.beginBatch();
    for (int i = 0; i < 50; i++) {
      const QString t = makeTrack(QStringLiteral("batch/%1.flac").arg(i));
      tracks << t;
      QVERIFY(s.put(t, 0, sampleGain()));
    }
    QCOMPARE(s.count(), qint64(50));
    s.endBatch();
  }
  ReplayGain::Store reopened(db);
  QCOMPARE(reopened.count(), qint64(50));
  for (const QString &t : tracks) {
    QVERIFY(reopened.get(t, 0).isValid());
  }
}

void TestReplayGainStore::pruneDropsEntriesForDeletedFiles() {
  const QString kept = makeTrack(QStringLiteral("album/keep.flac"));
  const QString gone = makeTrack(QStringLiteral("album/gone.flac"));
  const QString folder = QFileInfo(kept).absolutePath();

  ReplayGain::Store s(storeDir());
  QVERIFY(s.put(kept, 0, sampleGain()));
  QVERIFY(s.put(gone, 0, sampleGain()));
  QCOMPARE(s.count(), qint64(2));

  QVERIFY(QFile::remove(gone));
  QCOMPARE(s.pruneMissing({folder}), 1);
  QCOMPARE(s.count(), qint64(1));
  QVERIFY(s.get(kept, 0).isValid());
}

// An unmounted drive must not wipe the db.
void TestReplayGainStore::pruneKeepsEntriesWhenTheFolderIsGone() {
  const QString track = makeTrack(QStringLiteral("album/a.flac"));
  const QString folder = QFileInfo(track).absolutePath();

  ReplayGain::Store s(storeDir());
  QVERIFY(s.put(track, 0, sampleGain()));
  QVERIFY(QDir(folder).removeRecursively());

  QCOMPARE(s.pruneMissing({folder}), 0);
  QCOMPARE(s.count(), qint64(1));
}

void TestReplayGainStore::pruneIgnoresFoldersItWasNotGiven() {
  const QString one = makeTrack(QStringLiteral("one/a.flac"));
  const QString two = makeTrack(QStringLiteral("two/b.flac"));

  ReplayGain::Store s(storeDir());
  QVERIFY(s.put(one, 0, sampleGain()));
  QVERIFY(s.put(two, 0, sampleGain()));
  QVERIFY(QFile::remove(one));
  QVERIFY(QFile::remove(two));

  QCOMPARE(s.pruneMissing({QFileInfo(one).absolutePath()}), 1);
  QCOMPARE(s.count(), qint64(1));
}

void TestReplayGainStore::corruptFileIsQuarantinedNotFatal() {
  const QString db = storeDir();
  const QString path = db + QStringLiteral("/replaygain.sqlite");
  QFile f(path);
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write(QByteArray(4096, '\x7f')); // not a database
  f.close();

  ReplayGain::Store s(db);
  QVERIFY(s.isOpen());
  QCOMPARE(s.count(), qint64(0));

  const QString track = makeTrack(QStringLiteral("a.flac"));
  QVERIFY(s.put(track, 0, sampleGain()));
  QVERIFY(s.get(track, 0).isValid());

  // the damaged file is moved aside, never deleted
  const QStringList aside = QDir(db).entryList({QStringLiteral("replaygain.sqlite.corrupt-*")});
  QCOMPARE(aside.size(), 1);
}

void TestReplayGainStore::newerSchemaIsLeftAlone() {
  const QString db = storeDir();
  const QString track = makeTrack(QStringLiteral("a.flac"));
  {
    ReplayGain::Store s(db);
    QVERIFY(s.put(track, 0, sampleGain()));
  }

  sqlite3 *raw = nullptr;
  const QString path = db + QStringLiteral("/replaygain.sqlite");
  QCOMPARE(sqlite3_open(path.toUtf8().constData(), &raw), SQLITE_OK);
  QCOMPARE(sqlite3_exec(raw, "UPDATE meta SET value = '99' WHERE key = 'schema_version'",
                        nullptr, nullptr, nullptr), SQLITE_OK);
  sqlite3_close(raw);

  ReplayGain::Store reopened(db);
  QVERIFY(!reopened.isOpen());
  QVERIFY(!reopened.get(track, 0).isValid());
  QCOMPARE(reopened.count(), qint64(0));
}

QTEST_GUILESS_MAIN(TestReplayGainStore)
#include "tst_replaygainstore.moc"
