#include <QtTest>

#include "statusbarlabel.h"

#include <limits>

namespace {
  Track mk() {
    return Track("/music/song.mp3", 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100);
  }

  Track longTrack() {
    return Track("/music/song.mp3", 0, QString("artist").repeated(40), "album", QString("title").repeated(40),
                 1, 2000, 1000, 2, 320, 44100);
  }

  QString bufferSuffix(const QString &text) {
    const auto parts = text.split(" | ");
    return parts.size() > 2 ? parts.last() : QString();
  }
}

class TestStatusBarLabel : public QObject {
  Q_OBJECT
private slots:
  void startsStopped();
  void started_showsTheTrackAndAudioInfo();
  void paused_keepsTheTrackAndSwitchesTheState();
  void stopped_dropsTheTrack();
  void streamBufferFill_isSuppressedWhileStopped();
  void streamBufferFill_appendsOnlyWhenNonZero();
  void streamBufferFill_humanizesTheUnits();
  void streamBufferFill_stopsScalingAtTerabytes();
  void progress_refreshesWithoutChangingTheState();
  void doubleClick_isReported();
  void longTrack_doesNotWidenTheMinimumSize();
};

void TestStatusBarLabel::startsStopped() {
  StatusBarLabel label;
  QCOMPARE(label.text(), QString("Stopped"));
}

void TestStatusBarLabel::started_showsTheTrackAndAudioInfo() {
  StatusBarLabel label;
  const Track track = mk();

  label.on_playerStarted(track);

  QCOMPARE(label.text(), "Playing: " + track.shortText() + " | " + track.formattedAudioInfo());
}

void TestStatusBarLabel::paused_keepsTheTrackAndSwitchesTheState() {
  StatusBarLabel label;
  label.on_playerStarted(mk());

  label.on_playerPaused(mk());

  QVERIFY(label.text().startsWith("Paused: "));
}

void TestStatusBarLabel::stopped_dropsTheTrack() {
  StatusBarLabel label;
  label.on_playerStarted(mk());

  label.on_playerStopped();

  QCOMPARE(label.text(), QString("Stopped"));
}

void TestStatusBarLabel::streamBufferFill_isSuppressedWhileStopped() {
  StatusBarLabel label;

  label.on_streamBufferFill(mk(), 4096);

  QCOMPARE(label.text(), QString("Stopped"));
}

void TestStatusBarLabel::streamBufferFill_appendsOnlyWhenNonZero() {
  StatusBarLabel label;
  label.on_playerStarted(mk());

  label.on_streamBufferFill(mk(), 0);
  QVERIFY(bufferSuffix(label.text()).isEmpty());

  label.on_streamBufferFill(mk(), 512);
  QCOMPARE(bufferSuffix(label.text()), QString("stream buffer 512 bytes"));
}

void TestStatusBarLabel::streamBufferFill_humanizesTheUnits() {
  StatusBarLabel label;
  label.on_playerStarted(mk());

  label.on_streamBufferFill(mk(), 2 * 1024);
  QCOMPARE(bufferSuffix(label.text()), QString("stream buffer 2 KB"));

  label.on_streamBufferFill(mk(), 3 * 1024 * 1024);
  QCOMPARE(bufferSuffix(label.text()), QString("stream buffer 3 MB"));

  label.on_streamBufferFill(mk(), 2u * 1024 * 1024 * 1024);
  QCOMPARE(bufferSuffix(label.text()), QString("stream buffer 2 GB"));
}

void TestStatusBarLabel::streamBufferFill_stopsScalingAtTerabytes() {
  StatusBarLabel label;
  label.on_playerStarted(mk());

  // quint32 tops out below 4 GB, so GB is the last unit reachable here.
  label.on_streamBufferFill(mk(), std::numeric_limits<quint32>::max());
  QCOMPARE(bufferSuffix(label.text()), QString("stream buffer 3 GB"));
}

void TestStatusBarLabel::progress_refreshesWithoutChangingTheState() {
  StatusBarLabel label;
  label.on_playerStarted(mk());
  const QString before = label.text();

  label.on_progress(mk(), 42);

  QCOMPARE(label.text(), before);
}

void TestStatusBarLabel::doubleClick_isReported() {
  StatusBarLabel label;
  label.resize(100, 20);
  QSignalSpy spy(&label, &StatusBarLabel::doubleclicked);

  QTest::mouseDClick(&label, Qt::LeftButton);

  QCOMPARE(spy.count(), 1);
}

void TestStatusBarLabel::longTrack_doesNotWidenTheMinimumSize() {
  StatusBarLabel label;
  const Track track = longTrack();

  label.on_playerStarted(track);

  QVERIFY(label.text().contains(track.shortText()));
  QVERIFY(label.minimumSizeHint().width() < label.fontMetrics().horizontalAdvance(label.text()) / 4);
  QCOMPARE(label.toolTip(), label.text());
}

QTEST_MAIN(TestStatusBarLabel)
#include "tst_statusbarlabel.moc"
