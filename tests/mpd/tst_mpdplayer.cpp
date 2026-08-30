#include "server.h"

#include "mpd_client/client.h"
#include "playback/mpd/mediaplayer.h"
#include "playlist/mpdloader.h"

#include <QSignalSpy>
#include <QtTest>

namespace {
  const char *kPlaylist = "queue";
}

// Playback::Mpd::MediaPlayer against a real server. The base ctor builds a
// QMediaPlayer and QAudioOutput, so this binary needs an audio sink even though
// nothing here decodes locally.
class TestMpdPlayer : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void cleanup();

  void playStartsTheTrackFoundInTheMpdPlaylist();
  void playIsIgnoredWhenTheTrackIsNotInThePlaylist();
  void pauseTogglesThroughTheServer();
  void stopStopsTheServer();
  void positionFollowsTheServerElapsed();
  void positionChangedIsEmittedWhilePlaying();
  void setPositionSeeksInSeconds();
  void volumeMirrorsTheServerVolume();
  void trackChangedIsEmittedWhenTheServerSwitchesSong();
  void audioFormatUpdatedCarriesTheServerFormat();
  void durationChangedCarriesTheServerTotalTime();

private:
  QVector<Track> playlistTracks();

  MpdTest::Server server;
  std::unique_ptr<MpdClient::Client> client;
  std::unique_ptr<Playback::Mpd::MediaPlayer> player;
};

void TestMpdPlayer::initTestCase() {
  if (!MpdTest::Server::installed()) {
    QSKIP("mpd is not installed");
  }
  MpdTest::registerMetaTypes();
  QVERIFY2(server.start(), qPrintable(server.failReason()));

  client = std::make_unique<MpdClient::Client>();
  client->openConnection(server.url());
  QVERIFY(client->ping());

  player = std::make_unique<Playback::Mpd::MediaPlayer>(0, QByteArray(), *client);
}

void TestMpdPlayer::cleanupTestCase() {
  player.reset();
  client.reset();
}

void TestMpdPlayer::init() {
  QVERIFY(client->createPlaylist({"wav/long_a.wav", "wav/long_b.wav"}, kPlaylist));
}

void TestMpdPlayer::cleanup() {
  player->stop();
  player->clearTrack();
  server.resetState();
  QTest::qWait(50);
}

QVector<Track> TestMpdPlayer::playlistTracks() {
  return Playlist::MpdLoader(*client).playlistTracks(kPlaylist);
}

void TestMpdPlayer::playStartsTheTrackFoundInTheMpdPlaylist() {
  const auto tracks = playlistTracks();
  QCOMPARE(tracks.size(), 2);

  player->setTrack(tracks.at(1));
  player->play();

  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);
  QCOMPARE(client->status().songPos, 1);
  QCOMPARE(client->currentSong().filepath, QString("wav/long_b.wav"));
}

void TestMpdPlayer::playIsIgnoredWhenTheTrackIsNotInThePlaylist() {
  Track stranger("wav/nowhere.wav", 0, "a", "b", "c", 1, 2000, 1000, 2, 320, 44100);
  stranger.setMpd(server.url());
  stranger.setPlaylistName(kPlaylist);

  player->setTrack(stranger);
  player->play();

  QTest::qWait(300);
  QCOMPARE(client->status().state, MpdClient::Status::Stop);
  QCOMPARE(player->state(), Playback::MediaPlayer::StoppedState);
}

void TestMpdPlayer::pauseTogglesThroughTheServer() {
  player->setTrack(playlistTracks().at(0));
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);

  player->pause();
  QTRY_COMPARE(client->status().state, MpdClient::Status::Pause);
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PausedState);

  // A second play on a paused, unchanged track unpauses instead of reloading.
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);
}

void TestMpdPlayer::stopStopsTheServer() {
  player->setTrack(playlistTracks().at(0));
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);

  player->stop();

  QTRY_COMPARE(client->status().state, MpdClient::Status::Stop);
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::StoppedState);
}

void TestMpdPlayer::positionFollowsTheServerElapsed() {
  player->setTrack(playlistTracks().at(0));
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);

  client->setPosition(15);

  // The player extrapolates from the last status it saw, so it lands near the
  // server's elapsed rather than exactly on it.
  QTRY_VERIFY_WITH_TIMEOUT(player->position() >= 15000, 5000);
  QVERIFY(player->position() < 22000);
}

void TestMpdPlayer::positionChangedIsEmittedWhilePlaying() {
  player->setTrack(playlistTracks().at(0));
  QSignalSpy position(player.get(), &Playback::MediaPlayer::positionChanged);
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);

  // A 500 ms timer drives the progress updates.
  QTRY_VERIFY_WITH_TIMEOUT(position.count() >= 2, 5000);
  QVERIFY(position.last().at(0).toLongLong() >= 0);
}

void TestMpdPlayer::setPositionSeeksInSeconds() {
  player->setTrack(playlistTracks().at(0));
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);

  // The player takes milliseconds and mpd takes seconds.
  player->setPosition(12000);

  QTRY_VERIFY(client->status().elapsedMs >= 12000);
  QVERIFY(client->status().elapsedMs < 18000);
}

void TestMpdPlayer::volumeMirrorsTheServerVolume() {
  player->setVolume(42);

  QTRY_COMPARE(client->status().volume, 42);
  QTRY_COMPARE(player->volume(), 42);
}

void TestMpdPlayer::trackChangedIsEmittedWhenTheServerSwitchesSong() {
  player->setTrack(playlistTracks().at(0));
  player->play();
  QTRY_COMPARE(player->state(), Playback::MediaPlayer::PlayingState);

  QSignalSpy changed(player.get(), &Playback::Mpd::MediaPlayer::trackChanged);
  client->next();

  QTRY_VERIFY(changed.count() >= 1);
  QCOMPARE(changed.last().at(0).toString(), QString("wav/long_b.wav"));
}

void TestMpdPlayer::audioFormatUpdatedCarriesTheServerFormat() {
  player->setTrack(playlistTracks().at(0));
  QSignalSpy format(player.get(), &Playback::Mpd::MediaPlayer::audioFormatUpdated);

  player->play();

  QTRY_VERIFY(format.count() >= 1);
  QCOMPARE(format.last().at(0).toUInt(), 44100u);
  QCOMPARE(format.last().at(1).toUInt(), 2u);
}

void TestMpdPlayer::durationChangedCarriesTheServerTotalTime() {
  player->setTrack(playlistTracks().at(0));
  QSignalSpy duration(player.get(), &Playback::Mpd::MediaPlayer::durationChanged);

  player->play();

  QTRY_VERIFY(duration.count() >= 1);
  QCOMPARE(duration.last().at(0).toULongLong(), 30000ull);
}

QTEST_MAIN(TestMpdPlayer)

#include "tst_mpdplayer.moc"
