#include <QtTest>

#include "lyrics/lrcparser.h"

class TestLrcParser : public QObject {
  Q_OBJECT
private slots:
  void looksLikeLrcDetectsTimestamp();
  void looksLikeLrcRejectsPlainText();
  void looksLikeLrcRejectsEmpty();
  void stripTimestampsRemovesAndPreserves();
  void stripTimestampsDropsMetadataTagLines();
  void stripTimestampsTrimsLeadingTrailingBlanks();
  void stripTimestampsHandlesCrlf();
  void stripTimestampsAcceptsMillisecondColonSeparator();
};

void TestLrcParser::looksLikeLrcDetectsTimestamp() {
  QVERIFY(Lyrics::LrcParser::looksLikeLrc(QStringLiteral("[00:12.34]hello")));
  QVERIFY(Lyrics::LrcParser::looksLikeLrc(QStringLiteral("[01:02]foo")));
  QVERIFY(Lyrics::LrcParser::looksLikeLrc(QStringLiteral("noise\n[03:04.567]bar")));
}

void TestLrcParser::looksLikeLrcRejectsPlainText() {
  QVERIFY(!Lyrics::LrcParser::looksLikeLrc(QStringLiteral("just some lyrics")));
  QVERIFY(!Lyrics::LrcParser::looksLikeLrc(QStringLiteral("[ar:Artist]")));
  QVERIFY(!Lyrics::LrcParser::looksLikeLrc(QStringLiteral("[12]")));
}

void TestLrcParser::looksLikeLrcRejectsEmpty() {
  QVERIFY(!Lyrics::LrcParser::looksLikeLrc(QString()));
  QVERIFY(!Lyrics::LrcParser::looksLikeLrc(QStringLiteral("")));
}

void TestLrcParser::stripTimestampsRemovesAndPreserves() {
  const QString in  = QStringLiteral("[00:01.00]first line\n[00:02.50]second line");
  const QString out = Lyrics::LrcParser::stripTimestamps(in);
  QCOMPARE(out, QStringLiteral("first line\nsecond line"));
}

void TestLrcParser::stripTimestampsDropsMetadataTagLines() {
  const QString in =
    QStringLiteral("[ar:Some Artist]\n[ti:Some Title]\n[00:01.00]actual lyric");
  const QString out = Lyrics::LrcParser::stripTimestamps(in);
  QCOMPARE(out, QStringLiteral("actual lyric"));
}

void TestLrcParser::stripTimestampsTrimsLeadingTrailingBlanks() {
  const QString in = QStringLiteral("\n\n[00:01.00]a\n[00:02.00]b\n\n");
  const QString out = Lyrics::LrcParser::stripTimestamps(in);
  QCOMPARE(out, QStringLiteral("a\nb"));
}

void TestLrcParser::stripTimestampsHandlesCrlf() {
  const QString in  = QStringLiteral("[00:01.00]a\r\n[00:02.00]b\r\n");
  const QString out = Lyrics::LrcParser::stripTimestamps(in);
  QCOMPARE(out, QStringLiteral("a\nb"));
}

void TestLrcParser::stripTimestampsAcceptsMillisecondColonSeparator() {
  const QString in  = QStringLiteral("[00:01:500]hello");
  const QString out = Lyrics::LrcParser::stripTimestamps(in);
  QCOMPARE(out, QStringLiteral("hello"));
}

QTEST_GUILESS_MAIN(TestLrcParser)
#include "tst_lrcparser.moc"
