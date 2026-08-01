#include <QtTest>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "replaygain/store.h"

class TestReplayGainStore : public QObject {
  Q_OBJECT
private slots:
  void init();

  void roundTrip();
  void absentEntryIsInvalid();
  void lastLineWins();
  void tornTailIsSkipped();
  void shortLineIsSkipped();
  void unknownVersionIsIgnored();
  void pathWithTabAndNewline();
  void staleWhenSizeChanged();
  void staleWhenMtimeChanged();
  void albumOnlyEntry();
  void compactCollapsesDuplicates();

private:
  QString makeTrack(const QString &name, const QByteArray &content = "audio");
  static ReplayGain::Gain sampleGain();

  std::unique_ptr<QTemporaryDir> dir;
};

void TestReplayGainStore::init() {
  dir = std::make_unique<QTemporaryDir>();
  dir->setAutoRemove(true);
  QVERIFY(dir->isValid());
}

QString TestReplayGainStore::makeTrack(const QString &name, const QByteArray &content) {
  const QString path = dir->filePath(name);
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
  const QString track = makeTrack("01.flac");
  QVERIFY(!track.isEmpty());

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(track, 0, sampleGain()));
    QCOMPARE(s.count(), 1);
  }

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 1);
  const ReplayGain::Gain g = s.get(track, 0);
  QVERIFY(g.isValid());
  QVERIFY(g.has_track);
  QVERIFY(g.has_album);
  QCOMPARE(g.track_db, -5.57);
  QCOMPARE(g.track_peak, 0.910858);
  QCOMPARE(g.album_db, -2.48);
  QCOMPARE(g.album_peak, 0.977203);
}

void TestReplayGainStore::absentEntryIsInvalid() {
  const QString track = makeTrack("01.flac");
  ReplayGain::Store s(dir->path());
  QVERIFY(!s.get(track, 0).isValid());
  QVERIFY(!s.get(dir->filePath("nope.flac"), 0).isValid());
}

void TestReplayGainStore::lastLineWins() {
  const QString track = makeTrack("01.flac");

  {
    ReplayGain::Store s(dir->path());
    ReplayGain::Gain g = sampleGain();
    QVERIFY(s.put(track, 0, g));
    g.track_db = -9.99;
    QVERIFY(s.put(track, 0, g));
    QCOMPARE(s.count(), 1);
  }

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 1);
  QCOMPARE(s.get(track, 0).track_db, -9.99);
}

void TestReplayGainStore::tornTailIsSkipped() {
  const QString first = makeTrack("01.flac");
  const QString second = makeTrack("02.flac");

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(first, 0, sampleGain()));
    QVERIFY(s.put(second, 0, sampleGain()));
  }

  QFile f(dir->filePath("replaygain.db"));
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Append));
  f.write("-3.00\t0.5");
  f.close();

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 2);
  QVERIFY(s.get(first, 0).isValid());
  QVERIFY(s.get(second, 0).isValid());
}

void TestReplayGainStore::shortLineIsSkipped() {
  const QString track = makeTrack("01.flac");
  const QFileInfo info(track);

  QFile f(dir->filePath("replaygain.db"));
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.write("#mpz-rg 1\n");
  f.write("-1.00\t0.5\n");
  f.write(QString("-3.00\t0.500000\t\t\t%1\t%2\t0\t%3\n")
              .arg(QString::number(info.lastModified().toSecsSinceEpoch()),
                   QString::number(info.size()), track)
              .toUtf8());
  f.close();

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 1);
  const ReplayGain::Gain g = s.get(track, 0);
  QVERIFY(g.has_track);
  QVERIFY(!g.has_album);
  QCOMPARE(g.track_db, -3.0);
}

void TestReplayGainStore::unknownVersionIsIgnored() {
  const QString track = makeTrack("01.flac");

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(track, 0, sampleGain()));
  }

  QFile f(dir->filePath("replaygain.db"));
  QVERIFY(f.open(QIODevice::ReadOnly));
  QByteArray body = f.readAll();
  f.close();
  body.replace("#mpz-rg 1", "#mpz-rg 7");
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.write(body);
  f.close();

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 0);
  QVERIFY(!s.get(track, 0).isValid());
}

void TestReplayGainStore::pathWithTabAndNewline() {
  const QString track = makeTrack(QStringLiteral("od\td\nname.flac"));
  QVERIFY(!track.isEmpty());
  QVERIFY(QFileInfo::exists(track));

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(track, 0, sampleGain()));
  }

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 1);
  QVERIFY(s.get(track, 0).isValid());
}

void TestReplayGainStore::staleWhenSizeChanged() {
  const QString track = makeTrack("01.flac", "audio");

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(track, 0, sampleGain()));
  }

  QFile f(track);
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.write("audio-but-longer");
  f.close();

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 1);
  QVERIFY(!s.get(track, 0).isValid());
}

void TestReplayGainStore::staleWhenMtimeChanged() {
  const QString track = makeTrack("01.flac", "audio");

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(track, 0, sampleGain()));
  }

  QFile f(track);
  QVERIFY(f.open(QIODevice::ReadWrite));
  QVERIFY(f.setFileTime(QDateTime::currentDateTime().addSecs(-4242),
                        QFileDevice::FileModificationTime));
  f.close();

  ReplayGain::Store s(dir->path());
  QVERIFY(!s.get(track, 0).isValid());
}

void TestReplayGainStore::albumOnlyEntry() {
  const QString track = makeTrack("01.flac");

  ReplayGain::Gain g;
  g.album_db = -2.48;
  g.album_peak = 0.977203;
  g.has_album = true;

  {
    ReplayGain::Store s(dir->path());
    QVERIFY(s.put(track, 0, g));
  }

  ReplayGain::Store s(dir->path());
  const ReplayGain::Gain loaded = s.get(track, 0);
  QVERIFY(loaded.isValid());
  QVERIFY(!loaded.has_track);
  QVERIFY(loaded.has_album);
  QCOMPARE(loaded.album_db, -2.48);
}

void TestReplayGainStore::compactCollapsesDuplicates() {
  const QString track = makeTrack("01.flac");
  const QString db = dir->filePath("replaygain.db");

  {
    ReplayGain::Store s(dir->path());
    ReplayGain::Gain g = sampleGain();
    for (int i = 0; i < 10; i++) {
      g.track_db = -1.0 * i;
      QVERIFY(s.put(track, 0, g));
    }
    QCOMPARE(s.count(), 1);
    QVERIFY(s.compact());
  }

  QFile f(db);
  QVERIFY(f.open(QIODevice::ReadOnly));
  const QList<QByteArray> lines = f.readAll().trimmed().split('\n');
  f.close();
  QCOMPARE(lines.size(), 2);
  QCOMPARE(lines.at(0), QByteArray("#mpz-rg 1"));

  ReplayGain::Store s(dir->path());
  QCOMPARE(s.count(), 1);
  QCOMPARE(s.get(track, 0).track_db, -9.0);
}

QTEST_GUILESS_MAIN(TestReplayGainStore)
#include "tst_replaygainstore.moc"
