#include <QtTest>

#include "playback/gapless/timeline.h"

using Playback::Gapless::Timeline;

class TestGaplessTimeline : public QObject {
  Q_OBJECT
private slots:
  void contiguousSameFileBoundaries();
  void openTailAbsorption();
  void appendAfterOpenIsRejected();
  void closeSegmentShiftsLaterStarts();
  void removeLastSegmentDropsPreparedTail();
  void closeSegmentClampsToBegin();
  void absoluteForTrackMsClamping();
  void resetKeepsOneSegment();
  void emptyTimeline();
};

void TestGaplessTimeline::contiguousSameFileBoundaries() {
  Timeline tl;
  Track t1("/m/album.flac", 0);
  Track t2("/m/album.flac", 100);
  QUrl u("file:///m/album.flac");

  QCOMPARE(tl.appendSegment(t1, u, 0, 100), 0);
  QCOMPARE(tl.appendSegment(t2, u, 100, 250), 1);
  QCOMPARE(tl.segmentCount(), 2);

  QCOMPARE(tl.segmentStartAbs(0), qint64(0));
  QCOMPARE(tl.segmentEndAbs(0), qint64(100));
  QCOMPARE(tl.segmentStartAbs(1), qint64(100));
  QCOMPARE(tl.segmentEndAbs(1), qint64(250));

  const Timeline::Pos p99 = tl.map(99);
  QCOMPARE(p99.segment, 0);
  QCOMPARE(p99.track_frame, qint64(99));

  const Timeline::Pos p100 = tl.map(100);
  QCOMPARE(p100.segment, 1);
  QCOMPARE(p100.track_frame, qint64(100));

  const Timeline::Pos p249 = tl.map(249);
  QCOMPARE(p249.segment, 1);
  QCOMPARE(p249.track_frame, qint64(249));

  QCOMPARE(tl.map(250).segment, -1);

  QCOMPARE(tl.segmentUrl(1), u);
  QCOMPARE(tl.segmentBeginInFile(1), qint64(100));
  QCOMPARE(tl.segmentTrack(0).begin(), quint32(0));
  QCOMPARE(tl.segmentTrack(1).begin(), quint32(100));
  QCOMPARE(tl.segmentTrack(9).uid(), quint64(0));
}

void TestGaplessTimeline::openTailAbsorption() {
  Timeline tl;
  Track t("/m/song.flac", 0);
  QUrl u("file:///m/song.flac");

  tl.appendSegment(t, u, 0, -1);
  QCOMPARE(tl.segmentEndAbs(0), qint64(-1));

  const Timeline::Pos p = tl.map(500);
  QCOMPARE(p.segment, 0);
  QCOMPARE(p.track_frame, qint64(500));
}

void TestGaplessTimeline::appendAfterOpenIsRejected() {
  Timeline tl;
  Track t("/m/song.flac", 0);
  QUrl u("file:///m/song.flac");

  tl.appendSegment(t, u, 0, -1);
  QCOMPARE(tl.appendSegment(t, u, 0, 100), -1);
  QCOMPARE(tl.segmentCount(), 1);
}

void TestGaplessTimeline::closeSegmentShiftsLaterStarts() {
  Timeline tl;
  Track t1("/m/a.flac", 0);
  Track t2("/m/b.flac", 0);
  QUrl a("file:///m/a.flac"), b("file:///m/b.flac");

  tl.appendSegment(t1, a, 0, -1);
  tl.closeSegment(0, 200);
  QCOMPARE(tl.segmentEndAbs(0), qint64(200));

  tl.appendSegment(t2, b, 0, 300);
  QCOMPARE(tl.segmentStartAbs(1), qint64(200));
  QCOMPARE(tl.segmentEndAbs(1), qint64(500));

  tl.closeSegment(0, 180);
  QCOMPARE(tl.segmentEndAbs(0), qint64(180));
  QCOMPARE(tl.segmentStartAbs(1), qint64(180));
  QCOMPARE(tl.segmentEndAbs(1), qint64(480));

  QCOMPARE(tl.map(179).segment, 0);
  QCOMPARE(tl.map(180).segment, 1);
}

void TestGaplessTimeline::removeLastSegmentDropsPreparedTail() {
  Timeline tl;
  Track t1("/m/a.flac", 0);
  Track t2("/m/b.flac", 0);
  QUrl a("file:///m/a.flac"), b("file:///m/b.flac");

  tl.appendSegment(t1, a, 0, 200);
  tl.appendSegment(t2, b, 0, 300);
  QCOMPARE(tl.segmentCount(), 2);

  tl.removeLastSegment();
  QCOMPARE(tl.segmentCount(), 1);
  QCOMPARE(tl.segmentUrl(0), a);
  QCOMPARE(tl.segmentEndAbs(0), qint64(200));
  QCOMPARE(tl.map(250).segment, -1);

  tl.removeLastSegment();
  QCOMPARE(tl.segmentCount(), 0);
  tl.removeLastSegment();
  QCOMPARE(tl.segmentCount(), 0);
}

void TestGaplessTimeline::closeSegmentClampsToBegin() {
  Timeline tl;
  Track t("/m/a.flac", 0);
  QUrl u("file:///m/a.flac");

  tl.appendSegment(t, u, 50, 200);
  tl.closeSegment(0, 10);
  QCOMPARE(tl.segmentStartAbs(0), qint64(0));
  QCOMPARE(tl.segmentEndAbs(0), qint64(0));
}

void TestGaplessTimeline::absoluteForTrackMsClamping() {
  Timeline tl;
  Track t("/m/a.flac", 0);
  QUrl u("file:///m/a.flac");

  tl.appendSegment(t, u, 0, 44100);
  QCOMPARE(tl.absoluteForTrackMs(0, 500, 44100), qint64(22050));
  QCOMPARE(tl.absoluteForTrackMs(0, 2000, 44100), qint64(44100));
  QCOMPARE(tl.absoluteForTrackMs(0, -100, 44100), qint64(0));

  tl.appendSegment(t, u, 0, 44100);
  QCOMPARE(tl.absoluteForTrackMs(1, 500, 44100), qint64(66150));
  QCOMPARE(tl.absoluteForTrackMs(5, 0, 44100), qint64(-1));
}

void TestGaplessTimeline::resetKeepsOneSegment() {
  Timeline tl;
  Track t1("/m/a.flac", 0);
  Track t2("/m/b.flac", 0);
  QUrl a("file:///m/a.flac"), b("file:///m/b.flac");

  tl.appendSegment(t1, a, 0, 100);
  tl.appendSegment(t2, b, 0, 100);
  QCOMPARE(tl.segmentCount(), 2);

  tl.reset(t2, b, 10, 90);
  QCOMPARE(tl.segmentCount(), 1);
  QCOMPARE(tl.segmentUrl(0), b);
  QCOMPARE(tl.segmentBeginInFile(0), qint64(10));
  QCOMPARE(tl.segmentStartAbs(0), qint64(0));
  QCOMPARE(tl.segmentEndAbs(0), qint64(80));

  tl.clear();
  QCOMPARE(tl.segmentCount(), 0);
  QCOMPARE(tl.map(0).segment, -1);
}

void TestGaplessTimeline::emptyTimeline() {
  Timeline tl;
  QCOMPARE(tl.segmentCount(), 0);
  QCOMPARE(tl.map(0).segment, -1);
  QCOMPARE(tl.map(0).track_frame, qint64(0));
  QCOMPARE(tl.segmentStartAbs(0), qint64(-1));
  QCOMPARE(tl.segmentEndAbs(5), qint64(-1));
  QCOMPARE(tl.absoluteForTrackMs(0, 100, 44100), qint64(-1));
  QCOMPARE(tl.segmentUrl(0), QUrl());
  QCOMPARE(tl.segmentBeginInFile(0), qint64(0));
}

QTEST_GUILESS_MAIN(TestGaplessTimeline)
#include "tst_gaplesstimeline.moc"
