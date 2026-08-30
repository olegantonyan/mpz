#include "server.h"

#include "mpd_client/client.h"

#include <QSignalSpy>
#include <QtTest>

namespace {
  const char *kPlaylist = "queue";

  QStringList filepaths(const QVector<MpdClient::Song> &songs) {
    QStringList result;
    for (const auto &s : songs) {
      result << s.filepath;
    }
    return result;
  }
}

// Transport, status and queue commands. The two wav fixtures are 30 s each, so
// the null output plays them in real time and elapsed has room to move.
class TestMpdPlayback : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void play_loadsThePlaylistAndStartsAtTheGivenPosition();
  void play_returnsPromptlyForAnEmptyPlaylist();
  void status_reportsTheAudioFormatAndDurationWhilePlaying();
  void status_reportsTheNextSongForAMultiTrackQueue();
  void currentSong_returnsTheLoadedSong();
  void pause_thenUnpause_roundTrips();
  void stop_returnsToTheStoppedState();
  void next_andPrev_moveThroughTheQueue();
  void setPosition_seeksTheCurrentSong();
  void elapsedAdvancesInRealTime();
  void setVolume_isReflectedInStatusAndEmitsMixerChanged();
  void setRepeat_andSetRandom_areReflectedInStatusAndEmitOptionsChanged();
  void lsQueueSongs_returnsTheQueueWithIdsAndPositions();
  void setPriority_raisesTheSongInTheQueue();
  void resetAllPriorities_zeroesThem();
  void resetAllPriorities_isANoOpForAnEmptyQueue();
  void playerStateChangeFromAnotherClientEmitsPlayerStateChanged();
  void status_mapsTheSingleAndConsumeStates();

private:
  MpdTest::Server server;
  std::unique_ptr<MpdClient::Client> client;
};

void TestMpdPlayback::initTestCase() {
  if (!MpdTest::Server::installed()) {
    QSKIP("mpd is not installed");
  }
  MpdTest::registerMetaTypes();
  QVERIFY2(server.start(), qPrintable(server.failReason()));

  client = std::make_unique<MpdClient::Client>();
  client->openConnection(server.url());
  QVERIFY(client->ping());
}

void TestMpdPlayback::init() {
  QVERIFY(client->createPlaylist({"wav/long_a.wav", "wav/long_b.wav"}, kPlaylist));
}

void TestMpdPlayback::cleanup() {
  server.resetState();
  QTest::qWait(50);
}

void TestMpdPlayback::play_loadsThePlaylistAndStartsAtTheGivenPosition() {
  client->play(kPlaylist, 1);

  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);
  const auto status = client->status();
  QCOMPARE(status.songPos, 1);
  QCOMPARE(status.queueLength, 2);
  QCOMPARE(client->currentSong().filepath, QString("wav/long_b.wav"));
}

void TestMpdPlayback::play_returnsPromptlyForAnEmptyPlaylist() {
  QVERIFY(client->createPlaylist({}, "empty"));

  // play() runs clear + load + idle_mask on the command socket, and the idle
  // wait blocks for the full 8 s timeout if the queue never signals a change.
  QElapsedTimer clock;
  clock.start();
  client->play("empty", 0);
  QVERIFY(client->ping());

  QVERIFY2(clock.elapsed() < 3000, qPrintable(QString("play took %1 ms").arg(clock.elapsed())));
}

void TestMpdPlayback::status_reportsTheAudioFormatAndDurationWhilePlaying() {
  client->play(kPlaylist, 0);
  // The format only appears once the decoder has opened the file, a moment
  // after the state flips to Play.
  QTRY_VERIFY(client->status().audioFormat.sampleRate != 0);

  const auto status = client->status();
  QCOMPARE(status.audioFormat.sampleRate, 44100u);
  QCOMPARE(status.audioFormat.channels, 2u);
  QCOMPARE(status.audioFormat.bits, 16u);
  QCOMPARE(status.totalTime, 30);
  QVERIFY(status.bitrate > 0);
}

void TestMpdPlayback::status_reportsTheNextSongForAMultiTrackQueue() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  const auto status = client->status();
  QCOMPARE(status.nextSongPos, 1);
  QVERIFY(status.nextSongId >= 0);
  QVERIFY(status.songId >= 0);
  QVERIFY(status.songId != status.nextSongId);
}

void TestMpdPlayback::currentSong_returnsTheLoadedSong() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  const auto song = client->currentSong();
  QCOMPARE(song.filepath, QString("wav/long_a.wav"));
  QCOMPARE(song.pos, 0);
  QCOMPARE(song.duration, 30);
  QVERIFY(song.id >= 0);
}

void TestMpdPlayback::pause_thenUnpause_roundTrips() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  client->pause();
  QTRY_COMPARE(client->status().state, MpdClient::Status::Pause);

  client->unpause();
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);
}

void TestMpdPlayback::stop_returnsToTheStoppedState() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  client->stop();

  QTRY_COMPARE(client->status().state, MpdClient::Status::Stop);
  // mpd keeps the queue cursor where it was, so songPos survives a stop.
  QCOMPARE(client->status().songPos, 0);
  QCOMPARE(client->status().queueLength, 2);
}

void TestMpdPlayback::next_andPrev_moveThroughTheQueue() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().songPos, 0);

  client->next();
  QTRY_COMPARE(client->status().songPos, 1);
  QCOMPARE(client->currentSong().filepath, QString("wav/long_b.wav"));

  client->prev();
  QTRY_COMPARE(client->status().songPos, 0);
  QCOMPARE(client->currentSong().filepath, QString("wav/long_a.wav"));
}

void TestMpdPlayback::setPosition_seeksTheCurrentSong() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  // The argument is seconds, not milliseconds.
  client->setPosition(20);

  QTRY_VERIFY(client->status().elapsedMs >= 20000);
  QVERIFY(client->status().elapsedMs < 26000);
  QCOMPARE(client->status().songPos, 0);
}

void TestMpdPlayback::elapsedAdvancesInRealTime() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  const int first = client->status().elapsedMs;
  QTest::qWait(1500);

  // The null output keeps its timer on, so playback consumes real time.
  QVERIFY2(client->status().elapsedMs > first,
           qPrintable(QString("%1 did not advance past %2").arg(client->status().elapsedMs).arg(first)));
}

void TestMpdPlayback::setVolume_isReflectedInStatusAndEmitsMixerChanged() {
  QSignalSpy mixer(client.get(), &MpdClient::Client::mixerChanged);

  client->setVolume(37);

  QTRY_COMPARE(client->status().volume, 37);
  QTRY_VERIFY(mixer.count() >= 1);
}

void TestMpdPlayback::setRepeat_andSetRandom_areReflectedInStatusAndEmitOptionsChanged() {
  QSignalSpy options(client.get(), &MpdClient::Client::optionsChanged);

  client->setRepeat(true);
  QTRY_VERIFY(client->status().repeat);

  client->setRandom(true);
  QTRY_VERIFY(client->status().random);
  QTRY_VERIFY(options.count() >= 1);

  client->setRepeat(false);
  client->setRandom(false);
  QTRY_VERIFY(!client->status().repeat);
  QTRY_VERIFY(!client->status().random);
}

void TestMpdPlayback::lsQueueSongs_returnsTheQueueWithIdsAndPositions() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  const auto queue = client->lsQueueSongs();

  QCOMPARE(filepaths(queue), QStringList({"wav/long_a.wav", "wav/long_b.wav"}));
  QCOMPARE(queue.at(0).pos, 0);
  QCOMPARE(queue.at(1).pos, 1);
  QVERIFY(queue.at(0).id >= 0);
  QVERIFY(queue.at(1).id != queue.at(0).id);
  QCOMPARE(queue.at(0).prio, 0);
}

void TestMpdPlayback::setPriority_raisesTheSongInTheQueue() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  const auto before = client->lsQueueSongs();
  QCOMPARE(before.size(), 2);
  client->setPriority(before.at(1).id, 255);

  QTRY_COMPARE(client->lsQueueSongs().at(1).prio, 255);
  QCOMPARE(client->lsQueueSongs().at(0).prio, 0);
}

void TestMpdPlayback::resetAllPriorities_zeroesThem() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);
  const auto queue = client->lsQueueSongs();
  client->setPriority(queue.at(1).id, 255);
  QTRY_COMPARE(client->lsQueueSongs().at(1).prio, 255);

  client->resetAllPriorities();

  QTRY_COMPARE(client->lsQueueSongs().at(1).prio, 0);
}

void TestMpdPlayback::resetAllPriorities_isANoOpForAnEmptyQueue() {
  QCOMPARE(client->status().queueLength, 0);

  client->resetAllPriorities();

  QVERIFY(client->ping());
}

void TestMpdPlayback::playerStateChangeFromAnotherClientEmitsPlayerStateChanged() {
  client->play(kPlaylist, 0);
  QTRY_COMPARE(client->status().state, MpdClient::Status::Play);

  QSignalSpy state(client.get(), &MpdClient::Client::playerStateChanged);
  server.command("pause 1");

  QTRY_VERIFY(state.count() >= 1);
  QCOMPARE(client->status().state, MpdClient::Status::Pause);
}

void TestMpdPlayback::status_mapsTheSingleAndConsumeStates() {
  QCOMPARE(client->status().single, MpdClient::Status::SingleOff);
  QCOMPARE(client->status().consume, MpdClient::Status::ConsumeOff);

  server.command("single 1");
  server.command("consume 1");

  QTRY_COMPARE(client->status().single, MpdClient::Status::SingleOn);
  QCOMPARE(client->status().consume, MpdClient::Status::ConsumeOn);
}

QTEST_GUILESS_MAIN(TestMpdPlayback)

#include "tst_mpdplayback.moc"
