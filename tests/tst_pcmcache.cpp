#include <QtTest>

#include "playback/gapless/pcmcache.h"

using Playback::Gapless::PcmCache;

namespace {
  QByteArray framesData(int bpf, qint64 first_frame, qint64 count) {
    QByteArray b;
    b.resize(static_cast<int>(count * bpf));
    for (qint64 k = 0; k < count; ++k) {
      const char v = static_cast<char>((first_frame + k) & 0xFF);
      for (int j = 0; j < bpf; ++j) {
        b[static_cast<int>(k * bpf + j)] = v;
      }
    }
    return b;
  }

  void append(PcmCache &c, const QUrl &u, const QByteArray &d) {
    c.append(u, d.constData(), d.size());
  }
}

class TestPcmCache : public QObject {
  Q_OBJECT
private slots:
  void keepFromStartThenSlide();
  void crossChunkReadIsByteExact();
  void protectFloorPinsEverything();
  void protectFloorStopsEvictionAtFloor();
  void prebufferCapAndBudgetSplit();
  void promoteReallocatesAllowance();
  void dropEntryRestoresBudget();
  void unknownUrlRobustness();
  void clearResets();
};

void TestPcmCache::keepFromStartThenSlide() {
  QUrl u("file:///a");
  PcmCache c(64, 16);
  c.openEntry(u, 4);

  append(c, u, framesData(4, 0, 12));
  QVERIFY(c.wantsMoreData(u));

  append(c, u, framesData(4, 12, 4));
  QCOMPARE(c.firstFrame(u), qint64(0));
  QCOMPARE(c.frontierFrame(u), qint64(16));
  QVERIFY(c.contains(u, 0));
  QVERIFY(c.contains(u, 15));
  QVERIFY(!c.contains(u, 16));
  QCOMPARE(c.totalBytes(), qint64(64));

  append(c, u, framesData(4, 16, 4));
  QCOMPARE(c.firstFrame(u), qint64(4));
  QCOMPARE(c.frontierFrame(u), qint64(20));
  QVERIFY(!c.contains(u, 0));
  QVERIFY(!c.contains(u, 3));
  QVERIFY(c.contains(u, 4));
  QVERIFY(c.contains(u, 19));
  QCOMPARE(c.totalBytes(), qint64(64));
}

void TestPcmCache::crossChunkReadIsByteExact() {
  QUrl u("file:///a");
  PcmCache c(1024, 16);
  c.openEntry(u, 4);
  append(c, u, framesData(4, 0, 20));

  char buf[8 * 4];
  QCOMPARE(c.read(u, 3, buf, 8), qint64(8));
  for (qint64 k = 0; k < 8; ++k) {
    for (int j = 0; j < 4; ++j) {
      QCOMPARE(quint8(buf[k * 4 + j]), quint8((3 + k) & 0xFF));
    }
  }

  char buf2[10 * 4];
  QCOMPARE(c.read(u, 18, buf2, 10), qint64(2));
  QCOMPARE(c.read(u, 20, buf2, 1), qint64(0));
  QCOMPARE(c.read(u, 19, buf2, 4), qint64(1));
}

void TestPcmCache::protectFloorPinsEverything() {
  QUrl u("file:///a");
  PcmCache c(64, 16);
  c.openEntry(u, 4);
  c.protect(u, 0);

  append(c, u, framesData(4, 0, 12));
  QVERIFY(c.wantsMoreData(u));
  append(c, u, framesData(4, 12, 4));
  QVERIFY(!c.wantsMoreData(u));

  append(c, u, framesData(4, 16, 4));
  QCOMPARE(c.firstFrame(u), qint64(0));
  QCOMPARE(c.frontierFrame(u), qint64(20));
  QCOMPARE(c.totalBytes(), qint64(80));
  QVERIFY(!c.wantsMoreData(u));

  c.protect(u, 8);
  QVERIFY(c.wantsMoreData(u));
}

void TestPcmCache::protectFloorStopsEvictionAtFloor() {
  QUrl u("file:///a");
  PcmCache c(64, 16);
  c.openEntry(u, 4);
  c.protect(u, 8);

  append(c, u, framesData(4, 0, 24));
  QCOMPARE(c.frontierFrame(u), qint64(24));
  QCOMPARE(c.firstFrame(u), qint64(8));
  QCOMPARE(c.totalBytes(), qint64(64));

  char buf[4];
  QCOMPARE(c.read(u, 8, buf, 1), qint64(1));
  QCOMPARE(quint8(buf[0]), quint8(8));
}

void TestPcmCache::prebufferCapAndBudgetSplit() {
  QUrl a("file:///a"), b("file:///b");
  PcmCache c(64, 16);
  c.openEntry(a, 4);
  c.setPrebufferEntry(b, 4, 32);

  append(c, a, framesData(4, 0, 12));
  QCOMPARE(c.frontierFrame(a), qint64(12));
  QCOMPARE(c.firstFrame(a), qint64(4));

  append(c, b, framesData(4, 0, 12));
  QCOMPARE(c.frontierFrame(b), qint64(12));
  QCOMPARE(c.firstFrame(b), qint64(4));

  QCOMPARE(c.totalBytes(), qint64(64));
}

void TestPcmCache::promoteReallocatesAllowance() {
  QUrl a("file:///a"), b("file:///b");
  PcmCache c(64, 16);
  c.openEntry(a, 4);
  c.setPrebufferEntry(b, 4, 32);
  append(c, b, framesData(4, 0, 4));

  c.promote(b);
  QVERIFY(!c.contains(a, 0));
  QCOMPARE(c.frontierFrame(a), qint64(0));

  append(c, b, framesData(4, 4, 12));
  QCOMPARE(c.firstFrame(b), qint64(0));
  QCOMPARE(c.frontierFrame(b), qint64(16));
  QCOMPARE(c.totalBytes(), qint64(64));

  append(c, b, framesData(4, 16, 4));
  QCOMPARE(c.firstFrame(b), qint64(4));
  QCOMPARE(c.frontierFrame(b), qint64(20));
}

void TestPcmCache::dropEntryRestoresBudget() {
  QUrl a("file:///a"), b("file:///b");
  PcmCache c(64, 16);
  c.openEntry(a, 4);
  c.setPrebufferEntry(b, 4, 32);
  c.dropEntry(b);

  append(c, a, framesData(4, 0, 16));
  QCOMPARE(c.firstFrame(a), qint64(0));
  QCOMPARE(c.frontierFrame(a), qint64(16));
  QCOMPARE(c.totalBytes(), qint64(64));
}

void TestPcmCache::unknownUrlRobustness() {
  PcmCache c(64, 16);
  QUrl u("file:///nope");
  QCOMPARE(c.frontierFrame(u), qint64(0));
  QCOMPARE(c.firstFrame(u), qint64(0));
  QVERIFY(!c.contains(u, 0));
  QVERIFY(!c.wantsMoreData(u));

  char buf[16];
  QCOMPARE(c.read(u, 0, buf, 4), qint64(0));

  c.protect(u, 0);
  c.append(u, "abcd", 4);
  c.dropEntry(u);
  c.promote(u);
  QCOMPARE(c.totalBytes(), qint64(0));
}

void TestPcmCache::clearResets() {
  QUrl u("file:///a");
  PcmCache c(64, 16);
  c.openEntry(u, 4);
  append(c, u, framesData(4, 0, 8));
  QCOMPARE(c.totalBytes(), qint64(32));

  c.clear();
  QCOMPARE(c.totalBytes(), qint64(0));
  QVERIFY(!c.contains(u, 0));
  QCOMPARE(c.frontierFrame(u), qint64(0));
}

QTEST_GUILESS_MAIN(TestPcmCache)
#include "tst_pcmcache.moc"
