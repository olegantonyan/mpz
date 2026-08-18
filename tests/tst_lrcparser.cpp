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
  void handlesNetEaseSubIndexTimestamps();
  void dropsCreditLines();
  void keepsLyricsThatMentionCreditWords();
  void hasLyricContentRejectsCreditsOnly();
  void hasLyricContentRejectsInstrumentalPlaceholders();
  void hasLyricContentAcceptsRealLyrics();
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

void TestLrcParser::handlesNetEaseSubIndexTimestamps() {
  QVERIFY(Lyrics::LrcParser::looksLikeLrc(QStringLiteral("[00:00.00-1]hello")));
  const QString out = Lyrics::LrcParser::stripTimestamps(
    QStringLiteral("[00:12.34-1]first\n[00:56.78-12]second"));
  QCOMPARE(out, QStringLiteral("first\nsecond"));
}

void TestLrcParser::dropsCreditLines() {
  const QString in = QString::fromUtf8(
    "[00:00.00-1] 作词 : Vidar Jensen\n"
    "[00:00.00-1] 作曲 : Vidar Jensen\n"
    "[00:03.00]Produced by: Morfeus\n"
    "[00:07.00]I walk the night");
  QCOMPARE(Lyrics::LrcParser::toPlainLyrics(in), QStringLiteral("I walk the night"));

  const QString plain = QString::fromUtf8("作曲：Someone\nunder a black sun");
  QCOMPARE(Lyrics::LrcParser::toPlainLyrics(plain), QStringLiteral("under a black sun"));
}

void TestLrcParser::keepsLyricsThatMentionCreditWords() {
  const QString in = QStringLiteral("Written in the stars\nBass by the river\nDrums of war");
  QCOMPARE(Lyrics::LrcParser::toPlainLyrics(in), in);
}

void TestLrcParser::hasLyricContentRejectsCreditsOnly() {
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QString::fromUtf8(
    "[00:00.00-1] 作词 : Vidar Jensen\n[00:00.00-1] 作曲 : Vidar Jensen\n")));
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QStringLiteral("Lyricist: Someone")));
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QStringLiteral("[ar:Artist]\n[00:01.00]")));
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QStringLiteral("   \n\n")));
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QString()));
}

void TestLrcParser::hasLyricContentRejectsInstrumentalPlaceholders() {
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QString::fromUtf8("[00:05.00]纯音乐，请欣赏\n")));
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QString::fromUtf8(
    "此歌曲为没有填词的纯音乐，请您欣赏")));
  QVERIFY(!Lyrics::LrcParser::hasLyricContent(QString::fromUtf8("[00:00.00]暂无歌词")));
}

void TestLrcParser::hasLyricContentAcceptsRealLyrics() {
  QVERIFY(Lyrics::LrcParser::hasLyricContent(QStringLiteral("[00:01.00]a\n[00:02.00]b")));
  QVERIFY(Lyrics::LrcParser::hasLyricContent(QStringLiteral("just some lyrics")));
}

QTEST_GUILESS_MAIN(TestLrcParser)
#include "tst_lrcparser.moc"
