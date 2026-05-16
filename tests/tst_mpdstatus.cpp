#include <QtTest>

#include "mpd_client/status.h"

using MpdClient::Status;

class TestMpdStatus : public QObject {
  Q_OBJECT
private slots:
  void defaultsAreSentinelValues();
  void defaultAudioFormatIsZeroed();
  void nullStatusUpdateIsNoOp();
};

void TestMpdStatus::defaultsAreSentinelValues() {
  Status s;
  QCOMPARE(s.volume,       -1);
  QCOMPARE(s.repeat,       false);
  QCOMPARE(s.random,       false);
  QCOMPARE(s.single,       Status::SingleUnknown);
  QCOMPARE(s.consume,      Status::ConsumeUnknown);
  QCOMPARE(s.state,        Status::UnknownState);
  QCOMPARE(s.queueLength,  0);
  QCOMPARE(s.queueVersion, 0);
  QCOMPARE(s.crossfade,    0);
  QCOMPARE(s.mixRampDelay, 0);
  QCOMPARE(s.songPos,      -1);
  QCOMPARE(s.songId,       -1);
  QCOMPARE(s.nextSongPos,  -1);
  QCOMPARE(s.nextSongId,   -1);
  QCOMPARE(s.elapsedMs,    0);
  QCOMPARE(s.totalTime,    0);
  QCOMPARE(s.bitrate,      0);
  QCOMPARE(s.updateId,     0);
  QVERIFY(s.partition.isEmpty());
  QVERIFY(s.error.isEmpty());
}

void TestMpdStatus::defaultAudioFormatIsZeroed() {
  Status s;
  QCOMPARE(s.audioFormat.sampleRate, 0u);
  QCOMPARE(s.audioFormat.bits,       0u);
  QCOMPARE(s.audioFormat.channels,   0u);
}

void TestMpdStatus::nullStatusUpdateIsNoOp() {
  Status s;
  s.volume = 50;
  s.state  = Status::Play;
  s.updateFromMpdStatus(nullptr);
  QCOMPARE(s.volume, 50);
  QCOMPARE(s.state,  Status::Play);
}

QTEST_GUILESS_MAIN(TestMpdStatus)
#include "tst_mpdstatus.moc"
