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

QTEST_GUILESS_MAIN(TestStreamMetaData)
#include "tst_streammetadata.moc"
