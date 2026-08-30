#include <QtTest>
#include <QMimeData>

#include "tracksmimedata.h"

namespace {
  Track mk(const QString &path) {
    return Track(path, 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100);
  }
}

class TestTracksMimeData : public QObject {
  Q_OBJECT
private slots:
  void carriesItsOwnFormat();
  void roundTripsPayload();
  void from_rejectsAPlainMimeWithTheSameFormat();
  void from_handlesNull();
};

void TestTracksMimeData::carriesItsOwnFormat() {
  TracksMimeData mime({mk("/a.mp3")}, "album");
  QCOMPARE(TracksMimeData::format, QStringLiteral("application/x-mpz-tracks"));
  QVERIFY(mime.hasFormat(TracksMimeData::format));
}

void TestTracksMimeData::roundTripsPayload() {
  TracksMimeData mime({mk("/a.mp3"), mk("/b.mp3")}, "album", 42);

  QCOMPARE(mime.tracks().size(), 2);
  QCOMPARE(mime.tracks().at(1).path(), QString("/b.mp3"));
  QCOMPARE(mime.suggestedName(), QString("album"));
  QCOMPARE(mime.sourcePlaylistUid(), 42u);
}

void TestTracksMimeData::from_rejectsAPlainMimeWithTheSameFormat() {
  QMimeData plain;
  plain.setData(TracksMimeData::format, QByteArray());
  QCOMPARE(TracksMimeData::from(&plain), nullptr);

  TracksMimeData real({mk("/a.mp3")}, "album");
  QVERIFY(TracksMimeData::from(&real) != nullptr);
}

void TestTracksMimeData::from_handlesNull() {
  QCOMPARE(TracksMimeData::from(nullptr), nullptr);
}

QTEST_GUILESS_MAIN(TestTracksMimeData)
#include "tst_tracksmimedata.moc"
