#include <QtTest>

#include "playback/randomtrail.h"

class TestRandomTrail : public QObject {
  Q_OBJECT
private slots:
  void emptyTrailRecentlyPlayedIsFalse();
  void recentlyPlayedRespectsWindow();
  void recentlyPlayedZeroWindowIsFalse();
  void duplicateConsecutiveAddIsNoop();
  void forwardReplayAdvancesCursorInsteadOfBranching();
  void branchTruncatesAfterCursor();
  void goPrevAtStartReturnsZero();
  void goPrevWalksBackwardThroughTrail();
  void maxSizeDropsOldestAndShiftsCursor();
  void clearResets();
};

void TestRandomTrail::emptyTrailRecentlyPlayedIsFalse() {
  Playback::RandomTrail t;
  QVERIFY(!t.recentlyPlayed(1, 5));
}

void TestRandomTrail::recentlyPlayedRespectsWindow() {
  Playback::RandomTrail t;
  t.add(1);
  t.add(2);
  t.add(3);
  t.add(4);
  QVERIFY(t.recentlyPlayed(4, 1));
  QVERIFY(t.recentlyPlayed(3, 2));
  QVERIFY(!t.recentlyPlayed(2, 2));
  QVERIFY(t.recentlyPlayed(1, 4));
  QVERIFY(!t.recentlyPlayed(99, 4));
}

void TestRandomTrail::recentlyPlayedZeroWindowIsFalse() {
  Playback::RandomTrail t;
  t.add(1);
  QVERIFY(!t.recentlyPlayed(1, 0));
  QVERIFY(!t.recentlyPlayed(1, -3));
}

void TestRandomTrail::duplicateConsecutiveAddIsNoop() {
  Playback::RandomTrail t;
  t.add(7);
  t.add(7);
  QCOMPARE(t.goPrev(), quint64{0});
}

void TestRandomTrail::forwardReplayAdvancesCursorInsteadOfBranching() {
  Playback::RandomTrail t;
  t.add(1);
  t.add(2);
  t.add(3);
  QCOMPARE(t.goPrev(), quint64{2});
  QCOMPARE(t.goPrev(), quint64{1});
  t.add(2);
  QCOMPARE(t.goPrev(), quint64{1});
  QCOMPARE(t.goPrev(), quint64{0});
}

void TestRandomTrail::branchTruncatesAfterCursor() {
  Playback::RandomTrail t;
  t.add(1);
  t.add(2);
  t.add(3);
  QCOMPARE(t.goPrev(), quint64{2});
  t.add(9);
  QVERIFY(!t.recentlyPlayed(3, 10));
  QVERIFY(t.recentlyPlayed(9, 1));
}

void TestRandomTrail::goPrevAtStartReturnsZero() {
  Playback::RandomTrail t;
  QCOMPARE(t.goPrev(), quint64{0});
  t.add(42);
  QCOMPARE(t.goPrev(), quint64{0});
}

void TestRandomTrail::goPrevWalksBackwardThroughTrail() {
  Playback::RandomTrail t;
  t.add(10);
  t.add(20);
  t.add(30);
  QCOMPARE(t.goPrev(), quint64{20});
  QCOMPARE(t.goPrev(), quint64{10});
  QCOMPARE(t.goPrev(), quint64{0});
}

void TestRandomTrail::maxSizeDropsOldestAndShiftsCursor() {
  Playback::RandomTrail t(3);
  t.add(1);
  t.add(2);
  t.add(3);
  t.add(4);
  QVERIFY(!t.recentlyPlayed(1, 10));
  QVERIFY(t.recentlyPlayed(2, 3));
  QCOMPARE(t.goPrev(), quint64{3});
  QCOMPARE(t.goPrev(), quint64{2});
  QCOMPARE(t.goPrev(), quint64{0});
}

void TestRandomTrail::clearResets() {
  Playback::RandomTrail t;
  t.add(1);
  t.add(2);
  t.clear();
  QVERIFY(!t.recentlyPlayed(1, 10));
  QCOMPARE(t.goPrev(), quint64{0});
}

QTEST_GUILESS_MAIN(TestRandomTrail)
#include "tst_randomtrail.moc"
