#include <QtTest>

#include "playback/playerstate.h"

class TestPlayerState : public QObject {
  Q_OBJECT
private slots:
  void initialState();
  void setSelectedAndPlayingRoundTrip();
  void resetPlayingKeepsSelected();
  void followedCursorToggles();
};

void TestPlayerState::initialState() {
  Playback::PlayerState s;
  QCOMPARE(s.playingTrack(),  quint64{0});
  QCOMPARE(s.selectedTrack(), quint64{0});
  QVERIFY(!s.followedCursor());
}

void TestPlayerState::setSelectedAndPlayingRoundTrip() {
  Playback::PlayerState s;
  s.setSelected(7);
  s.setPlaying(42);
  QCOMPARE(s.selectedTrack(), quint64{7});
  QCOMPARE(s.playingTrack(),  quint64{42});
}

void TestPlayerState::resetPlayingKeepsSelected() {
  Playback::PlayerState s;
  s.setSelected(7);
  s.setPlaying(42);
  s.resetPlaying();
  QCOMPARE(s.playingTrack(),  quint64{0});
  QCOMPARE(s.selectedTrack(), quint64{7});
}

void TestPlayerState::followedCursorToggles() {
  Playback::PlayerState s;
  QVERIFY(!s.followedCursor());
  s.setFollowedCursor();
  QVERIFY(s.followedCursor());
  s.resetFolowedCursor();
  QVERIFY(!s.followedCursor());
}

QTEST_GUILESS_MAIN(TestPlayerState)
#include "tst_playerstate.moc"
