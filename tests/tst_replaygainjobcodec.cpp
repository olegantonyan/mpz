#include <QtTest>

#include "replaygain/jobcodec.h"

using namespace ReplayGain;

namespace {
  Job sampleJob() {
    Job job;
    job.epoch = 7;
    job.folder = QStringLiteral("/music/Ø Album (2001)");
    job.want_album = true;
    job.write_tags = true;

    FileWork whole;
    whole.path = QStringLiteral("/music/Ø Album (2001)/01 — track.flac");
    whole.slices.append(Slice{0, 0});

    FileWork cue;
    cue.path = QStringLiteral("/music/Ø Album (2001)/whole.ape");
    cue.slices.append(Slice{0, 210000});
    cue.slices.append(Slice{210000, 185500});

    job.files << whole << cue;
    return job;
  }

  JobResult sampleResult() {
    JobResult result;
    result.epoch = 7;
    result.folder = QStringLiteral("/music/Ø Album (2001)");

    SliceResult ok;
    ok.path = QStringLiteral("/music/Ø Album (2001)/01 — track.flac");
    ok.begin_ms = 0;
    ok.ok = true;
    ok.gain.track_db = -3.25;
    ok.gain.track_peak = 0.987654321;
    ok.gain.album_db = -2.5;
    ok.gain.album_peak = 1.0009;
    ok.gain.has_track = true;
    ok.gain.has_album = true;
    ok.tag_result = 2;

    SliceResult bad;
    bad.path = QStringLiteral("/music/Ø Album (2001)/whole.ape");
    bad.begin_ms = 210000;
    bad.error = QStringLiteral("channel count changes mid-file (2 -> 1)");

    result.slices << ok << bad;
    return result;
  }
}

class TestReplayGainJobCodec : public QObject {
  Q_OBJECT
private slots:
  void jobSurvivesARoundTrip();
  void resultSurvivesARoundTrip();
  void truncatedJobIsRejected();
  void messagesArePoppedInOrder();
  void partialBufferYieldsNothing();
};

void TestReplayGainJobCodec::jobSurvivesARoundTrip() {
  const Job sent = sampleJob();
  Job got;
  QVERIFY(decodeJob(encodeJob(sent), got));

  QCOMPARE(got.epoch, sent.epoch);
  QCOMPARE(got.folder, sent.folder);
  QCOMPARE(got.want_album, sent.want_album);
  QCOMPARE(got.write_tags, sent.write_tags);
  QCOMPARE(got.files.size(), sent.files.size());
  for (int i = 0; i < got.files.size(); i++) {
    QCOMPARE(got.files.at(i).path, sent.files.at(i).path);
    QCOMPARE(got.files.at(i).slices.size(), sent.files.at(i).slices.size());
    for (int s = 0; s < got.files.at(i).slices.size(); s++) {
      QCOMPARE(got.files.at(i).slices.at(s).begin_ms, sent.files.at(i).slices.at(s).begin_ms);
      QCOMPARE(got.files.at(i).slices.at(s).duration_ms,
               sent.files.at(i).slices.at(s).duration_ms);
    }
  }
}

void TestReplayGainJobCodec::resultSurvivesARoundTrip() {
  const JobResult sent = sampleResult();
  QByteArray buffer = frameJobDone(sent);

  Message type = Message::FileStarted;
  QByteArray payload;
  QVERIFY(takeMessage(buffer, type, payload));
  QCOMPARE(type, Message::JobDone);

  JobResult got;
  QVERIFY(decodeJobDone(payload, got));
  QCOMPARE(got.epoch, sent.epoch);
  QCOMPARE(got.folder, sent.folder);
  QCOMPARE(got.slices.size(), sent.slices.size());

  const SliceResult &a = got.slices.at(0);
  QCOMPARE(a.path, sent.slices.at(0).path);
  QVERIFY(a.ok);
  QCOMPARE(a.tag_result, 2);
  QCOMPARE(a.gain.track_db, sent.slices.at(0).gain.track_db);
  QCOMPARE(a.gain.track_peak, sent.slices.at(0).gain.track_peak);
  QCOMPARE(a.gain.album_peak, sent.slices.at(0).gain.album_peak);
  QVERIFY(a.gain.has_album);

  const SliceResult &b = got.slices.at(1);
  QVERIFY(!b.ok);
  QCOMPARE(b.begin_ms, quint64(210000));
  QCOMPARE(b.error, sent.slices.at(1).error);
}

void TestReplayGainJobCodec::truncatedJobIsRejected() {
  QByteArray encoded = encodeJob(sampleJob());
  encoded.chop(encoded.size() / 2);
  Job got;
  QVERIFY(!decodeJob(encoded, got));
  QVERIFY(!decodeJob(QByteArray(), got));
}

void TestReplayGainJobCodec::messagesArePoppedInOrder() {
  QByteArray buffer;
  buffer.append(frameFileStarted(QStringLiteral("/a/first.mp3")));
  buffer.append(frameFileStarted(QStringLiteral("/a/second.mp3")));
  buffer.append(frameJobDone(sampleResult()));

  Message type = Message::JobDone;
  QByteArray payload;

  QVERIFY(takeMessage(buffer, type, payload));
  QCOMPARE(type, Message::FileStarted);
  QCOMPARE(decodeFileStarted(payload), QStringLiteral("/a/first.mp3"));

  QVERIFY(takeMessage(buffer, type, payload));
  QCOMPARE(type, Message::FileStarted);
  QCOMPARE(decodeFileStarted(payload), QStringLiteral("/a/second.mp3"));

  QVERIFY(takeMessage(buffer, type, payload));
  QCOMPARE(type, Message::JobDone);

  QVERIFY(!takeMessage(buffer, type, payload));
  QVERIFY(buffer.isEmpty());
}

// The worker's output arrives in whatever chunks the pipe hands over, so a
// half-delivered message has to be left alone rather than half-parsed.
void TestReplayGainJobCodec::partialBufferYieldsNothing() {
  const QByteArray whole = frameFileStarted(QStringLiteral("/a/track.mp3"));
  Message type = Message::JobDone;
  QByteArray payload;

  for (int cut = 1; cut < whole.size(); cut++) {
    QByteArray partial = whole.left(cut);
    QVERIFY2(!takeMessage(partial, type, payload), qPrintable(QString("cut at %1").arg(cut)));
    QCOMPARE(partial.size(), cut);
  }

  QByteArray complete = whole;
  QVERIFY(takeMessage(complete, type, payload));
  QCOMPARE(decodeFileStarted(payload), QStringLiteral("/a/track.mp3"));
}

QTEST_GUILESS_MAIN(TestReplayGainJobCodec)
#include "tst_replaygainjobcodec.moc"
