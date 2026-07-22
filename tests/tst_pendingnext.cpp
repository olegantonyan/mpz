#include <QtTest>

#include "playback/pendingnext.h"

using Playback::PendingNext;

class TestPendingNext : public QObject {
  Q_OBJECT
private slots:
  void emptyIsInvalid();
  void validOnlyForMatchingUid();
  void takeReturnsTrackAndClears();
  void clearInvalidates();
  void secondSetOverwrites();
};

void TestPendingNext::emptyIsInvalid() {
  PendingNext p;
  QVERIFY(!p.validFor(0));
  QVERIFY(!p.validFor(42));
  QCOMPARE(p.boundaryUid(), quint64{0});
}

void TestPendingNext::validOnlyForMatchingUid() {
  PendingNext p;
  Track next("/m/next.flac", 0);
  p.set(42, next);
  QVERIFY(p.validFor(42));
  QVERIFY(!p.validFor(7));
  QVERIFY(!p.validFor(0));
  QCOMPARE(p.boundaryUid(), quint64{42});
}

void TestPendingNext::takeReturnsTrackAndClears() {
  PendingNext p;
  Track next("/m/next.flac", 0);
  p.set(42, next);
  Track taken = p.take();
  QCOMPARE(taken.uid(), next.uid());
  QCOMPARE(taken.path(), next.path());
  QVERIFY(!p.validFor(42));
  QCOMPARE(p.boundaryUid(), quint64{0});
}

void TestPendingNext::clearInvalidates() {
  PendingNext p;
  Track next("/m/next.flac", 0);
  p.set(42, next);
  p.clear();
  QVERIFY(!p.validFor(42));
  QCOMPARE(p.boundaryUid(), quint64{0});
}

void TestPendingNext::secondSetOverwrites() {
  PendingNext p;
  Track first("/m/first.flac", 0);
  Track second("/m/second.flac", 0);
  p.set(1, first);
  p.set(2, second);
  QVERIFY(!p.validFor(1));
  QVERIFY(p.validFor(2));
  QCOMPARE(p.boundaryUid(), quint64{2});
  Track taken = p.take();
  QCOMPARE(taken.uid(), second.uid());
}

QTEST_GUILESS_MAIN(TestPendingNext)
#include "tst_pendingnext.moc"
