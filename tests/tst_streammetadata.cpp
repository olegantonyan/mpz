#include <QtTest>

#include "streammetadata.h"

class TestStreamMetaData : public QObject {
  Q_OBJECT
private slots:
  void defaultIsEmpty();
  void insertMakesNonEmpty();
  void clearEmpties();
  void bitrateAndSamplerateParse();
  void bitrateInvalidReturnsZero();
  void artistAndTitleFromStreamTitle();
  void formatFromContentType();
  void explicitTitleCtorOverridesArtistAndTitle();
  void statusFallbackUsedWhenStreamTitleEmpty();
  void statusFallbackUsedWhenNoStreamKey();
  void inlineStreamTitleWinsOverStatusFallback();
  void statusFallbackSplitsArtistAndTitle();
  void explicitTitleStillOverridesStatusFallback();
  void statusFallbackAffectsIsEmptyAndClear();
};

void TestStreamMetaData::defaultIsEmpty() {
  StreamMetaData m;
  QVERIFY(m.isEmpty());
  QCOMPARE(m.bitrate(),    quint16{0});
  QCOMPARE(m.samplerate(), quint32{0});
  QCOMPARE(m.artist(),     QString());
  QCOMPARE(m.title(),      QString());
  QCOMPARE(m.format(),     QString());
}

void TestStreamMetaData::insertMakesNonEmpty() {
  StreamMetaData m;
  m.insert(QStringLiteral("icy-br"), QStringLiteral("128"));
  QVERIFY(!m.isEmpty());
}

void TestStreamMetaData::clearEmpties() {
  StreamMetaData m;
  m.insert(QStringLiteral("icy-br"), QStringLiteral("128"));
  m.clear();
  QVERIFY(m.isEmpty());
}

void TestStreamMetaData::bitrateAndSamplerateParse() {
  StreamMetaData m;
  m.insert(QStringLiteral("icy-br"), QStringLiteral("320"));
  m.insert(QStringLiteral("icy-sr"), QStringLiteral("44100"));
  QCOMPARE(m.bitrate(),    quint16{320});
  QCOMPARE(m.samplerate(), quint32{44100});
}

void TestStreamMetaData::bitrateInvalidReturnsZero() {
  StreamMetaData m;
  m.insert(QStringLiteral("icy-br"), QStringLiteral("not-a-number"));
  QCOMPARE(m.bitrate(), quint16{0});
}

void TestStreamMetaData::artistAndTitleFromStreamTitle() {
  StreamMetaData m;
  m.insert(QStringLiteral("stream"),
           QStringLiteral("StreamTitle='Some Artist - Some Title';"));
  QCOMPARE(m.artist(), QStringLiteral("Some Artist"));
  QCOMPARE(m.title(),  QStringLiteral("Some Title"));
}

void TestStreamMetaData::formatFromContentType() {
  StreamMetaData m;
  m.insert(QStringLiteral("content-type"), QStringLiteral("audio/mpeg"));
  QCOMPARE(m.format(), QStringLiteral("audio/mpeg"));
}

void TestStreamMetaData::explicitTitleCtorOverridesArtistAndTitle() {
  StreamMetaData m(QStringLiteral("Just A Title"));
  m.insert(QStringLiteral("stream"),
           QStringLiteral("StreamTitle='Ignored Artist - Ignored Title';"));
  QCOMPARE(m.artist(), QStringLiteral("Just A Title"));
  QCOMPARE(m.title(),  QStringLiteral("Just A Title"));
}

void TestStreamMetaData::statusFallbackUsedWhenStreamTitleEmpty() {
  StreamMetaData m;
  m.insert(QStringLiteral("stream"), QStringLiteral("StreamTitle='';"));
  m.setStatusNowPlaying(QStringLiteral("Vertical Mode - Time Machine"));
  QCOMPARE(m.artist(), QStringLiteral("Vertical Mode"));
  QCOMPARE(m.title(),  QStringLiteral("Time Machine"));
}

void TestStreamMetaData::statusFallbackUsedWhenNoStreamKey() {
  StreamMetaData m;
  m.setStatusNowPlaying(QStringLiteral("Kosta - What About The Machine"));
  QCOMPARE(m.artist(), QStringLiteral("Kosta"));
  QCOMPARE(m.title(),  QStringLiteral("What About The Machine"));
}

void TestStreamMetaData::inlineStreamTitleWinsOverStatusFallback() {
  StreamMetaData m;
  m.insert(QStringLiteral("stream"), QStringLiteral("StreamTitle='Live Artist - Live Title';"));
  m.setStatusNowPlaying(QStringLiteral("Stale Artist - Stale Title"));
  QCOMPARE(m.artist(), QStringLiteral("Live Artist"));
  QCOMPARE(m.title(),  QStringLiteral("Live Title"));
}

void TestStreamMetaData::statusFallbackSplitsArtistAndTitle() {
  StreamMetaData m;
  m.setStatusNowPlaying(QStringLiteral("NoSeparatorHere"));
  QCOMPARE(m.artist(), QStringLiteral("NoSeparatorHere"));
  QCOMPARE(m.title(),  QStringLiteral("NoSeparatorHere"));
}

void TestStreamMetaData::explicitTitleStillOverridesStatusFallback() {
  StreamMetaData m(QStringLiteral("Just A Title"));
  m.setStatusNowPlaying(QStringLiteral("Ignored Artist - Ignored Title"));
  QCOMPARE(m.artist(), QStringLiteral("Just A Title"));
  QCOMPARE(m.title(),  QStringLiteral("Just A Title"));
}

void TestStreamMetaData::statusFallbackAffectsIsEmptyAndClear() {
  StreamMetaData m;
  QVERIFY(m.isEmpty());
  m.setStatusNowPlaying(QStringLiteral("A - B"));
  QVERIFY(!m.isEmpty());
  m.clear();
  QVERIFY(m.isEmpty());
  QCOMPARE(m.title(), QString());
}

QTEST_GUILESS_MAIN(TestStreamMetaData)
#include "tst_streammetadata.moc"
