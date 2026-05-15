#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "playlist/cueparser.h"
#include "track.h"

using Playlist::CueParser;

namespace {
  bool writeFile(const QString &path, const QByteArray &content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
      return false;
    }
    return f.write(content) == content.size();
  }

  bool touch(const QString &path) {
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.flush();
  }
}

class TestCueParser : public QObject {
  Q_OBJECT
private slots:
  void init();

  void singleFileMultipleTracks();
  void crlfLineEndingsHandled();
  void utf8BomStripped();
  void nonAudioTrackTypeIsSkipped();
  void binaryFileBlockIsSkipped();
  void stemFallbackResolvesDifferentExtension();
  void missingAudioFileSkipsEntireBlock();
  void remDateExtractsYear();
  void unquotedAndQuotedFilesBothParse();
  void indexPositionMmSsFfConvertsToMs();
  void emptyCueReturnsNothing();
  void onlyTitleNoTracksReturnsNothing();

private:
  QTemporaryDir tempDir;
};

void TestCueParser::init() {
  // QTemporaryDir is a member and constructed once per test object, so files
  // accumulate across slots without this reset. resolve_audio_file's stem
  // fallback iterates the dir in QDir's default IgnoreCase sort — leftover
  // entries that case-collide with a newly created fixture can be returned
  // first, flipping the resolved path between runs.
  tempDir = QTemporaryDir{};
  QVERIFY(tempDir.isValid());
}

void TestCueParser::singleFileMultipleTracks() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("album.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("album.cue"));
  QVERIFY(writeFile(cue,
    "PERFORMER \"Some Band\"\n"
    "TITLE \"The Album\"\n"
    "FILE \"album.flac\" WAVE\n"
    "  TRACK 01 AUDIO\n"
    "    TITLE \"First\"\n"
    "    INDEX 01 00:00:00\n"
    "  TRACK 02 AUDIO\n"
    "    TITLE \"Second\"\n"
    "    INDEX 01 03:00:00\n"
    "  TRACK 03 AUDIO\n"
    "    TITLE \"Third\"\n"
    "    INDEX 01 06:30:00\n"));

  CueParser p(cue);
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 3);
  QCOMPARE(tracks.at(0).title(),        QStringLiteral("First"));
  QCOMPARE(tracks.at(1).title(),        QStringLiteral("Second"));
  QCOMPARE(tracks.at(2).title(),        QStringLiteral("Third"));
  QCOMPARE(tracks.at(0).artist(),       QStringLiteral("Some Band"));
  QCOMPARE(tracks.at(0).album(),        QStringLiteral("The Album"));
  QCOMPARE(tracks.at(0).begin(),        quint32{0});
  QCOMPARE(tracks.at(1).begin(),        quint32{180000});  // 3:00 in ms
  QCOMPARE(tracks.at(2).begin(),        quint32{390000});  // 6:30 in ms
  QCOMPARE(tracks.at(1).duration(),     quint32{210000});  // 6:30 - 3:00
  for (const Track &t : tracks) {
    QVERIFY(t.isCue());
  }
}

void TestCueParser::crlfLineEndingsHandled() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("song.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("crlf.cue"));
  QVERIFY(writeFile(cue,
    "FILE \"song.flac\" WAVE\r\n"
    "  TRACK 01 AUDIO\r\n"
    "    INDEX 01 00:00:00\r\n"));

  CueParser p(cue);
  QCOMPARE(p.tracks_list().size(), 1);
}

void TestCueParser::utf8BomStripped() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("bom.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("bom.cue"));
  QByteArray bytes;
  bytes.append("\xEF\xBB\xBF");  // UTF-8 BOM
  bytes.append("FILE \"bom.flac\" WAVE\n"
               "  TRACK 01 AUDIO\n"
               "    INDEX 01 00:00:00\n");
  QVERIFY(writeFile(cue, bytes));

  CueParser p(cue);
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 1);
}

void TestCueParser::nonAudioTrackTypeIsSkipped() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("mixed.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("mixed.cue"));
  QVERIFY(writeFile(cue,
    "FILE \"mixed.flac\" WAVE\n"
    "  TRACK 01 MODE1/2352\n"
    "    INDEX 01 00:00:00\n"
    "  TRACK 02 AUDIO\n"
    "    INDEX 01 00:30:00\n"));

  CueParser p(cue);
  // Only the AUDIO track survives.
  QCOMPARE(p.tracks_list().size(), 1);
}

void TestCueParser::binaryFileBlockIsSkipped() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("data.bin"))));
  QVERIFY(touch(tempDir.filePath(QStringLiteral("audio.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("split.cue"));
  QVERIFY(writeFile(cue,
    "FILE \"data.bin\" BINARY\n"
    "  TRACK 01 AUDIO\n"
    "    INDEX 01 00:00:00\n"
    "FILE \"audio.flac\" WAVE\n"
    "  TRACK 02 AUDIO\n"
    "    INDEX 01 00:00:00\n"));

  CueParser p(cue);
  auto tracks = p.tracks_list();
  // First FILE was BINARY → skipped; second FILE contributes one track.
  QCOMPARE(tracks.size(), 1);
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("audio.flac")));
}

void TestCueParser::stemFallbackResolvesDifferentExtension() {
  // CUE says "Song.wav" but the actual file on disk is "Song.flac".
  QVERIFY(touch(tempDir.filePath(QStringLiteral("Song.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("stem.cue"));
  QVERIFY(writeFile(cue,
    "FILE \"Song.wav\" WAVE\n"
    "  TRACK 01 AUDIO\n"
    "    INDEX 01 00:00:00\n"));

  CueParser p(cue);
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 1);
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("Song.flac")));
}

void TestCueParser::missingAudioFileSkipsEntireBlock() {
  const QString cue = tempDir.filePath(QStringLiteral("missing.cue"));
  QVERIFY(writeFile(cue,
    "FILE \"ghost.flac\" WAVE\n"
    "  TRACK 01 AUDIO\n"
    "    INDEX 01 00:00:00\n"));

  CueParser p(cue);
  QCOMPARE(p.tracks_list().size(), 0);
}

void TestCueParser::remDateExtractsYear() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("dated.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("dated.cue"));
  QVERIFY(writeFile(cue,
    "REM DATE 2017\n"
    "FILE \"dated.flac\" WAVE\n"
    "  TRACK 01 AUDIO\n"
    "    INDEX 01 00:00:00\n"));

  CueParser p(cue);
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.first().year(), quint16{2017});
}

void TestCueParser::unquotedAndQuotedFilesBothParse() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("noquote.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("unq.cue"));
  // No quotes around the filename — tokenizer should still pick it up.
  QVERIFY(writeFile(cue,
    "FILE noquote.flac WAVE\n"
    "  TRACK 01 AUDIO\n"
    "    INDEX 01 00:00:00\n"));

  CueParser p(cue);
  QCOMPARE(p.tracks_list().size(), 1);
}

void TestCueParser::indexPositionMmSsFfConvertsToMs() {
  QVERIFY(touch(tempDir.filePath(QStringLiteral("ms.flac"))));
  const QString cue = tempDir.filePath(QStringLiteral("ms.cue"));
  // 75 frames = 1 second. 00:01:75 is invalid (75 frames don't exist),
  // but 00:01:37 ≈ 1.493 seconds → 1493 ms.
  QVERIFY(writeFile(cue,
    "FILE \"ms.flac\" WAVE\n"
    "  TRACK 01 AUDIO\n"
    "    INDEX 01 00:00:00\n"
    "  TRACK 02 AUDIO\n"
    "    INDEX 01 00:01:37\n"));

  CueParser p(cue);
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 2);
  QCOMPARE(tracks.at(1).begin(), quint32{1493});  // (60+37)/75 * 1000
}

void TestCueParser::emptyCueReturnsNothing() {
  const QString cue = tempDir.filePath(QStringLiteral("empty.cue"));
  QVERIFY(touch(cue));
  CueParser p(cue);
  QCOMPARE(p.tracks_list().size(), 0);
}

void TestCueParser::onlyTitleNoTracksReturnsNothing() {
  const QString cue = tempDir.filePath(QStringLiteral("notrack.cue"));
  QVERIFY(writeFile(cue,
    "TITLE \"Album Only\"\n"
    "PERFORMER \"Artist Only\"\n"));
  CueParser p(cue);
  QCOMPARE(p.tracks_list().size(), 0);
}

QTEST_GUILESS_MAIN(TestCueParser)
#include "tst_cueparser.moc"
