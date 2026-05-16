#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "playlist/fileparser.h"
#include "track.h"

using Playlist::FileParser;

namespace {
  // Write content to a path; QVERIFY-style fatal if it fails.
  bool writeFile(const QString &path, const QByteArray &content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      return false;
    }
    return f.write(content) == content.size();
  }

  // Create an empty (zero-byte) file. Existence is enough to make Track ctors
  // and FileParser accept the entry; TagLib silently returns null on bogus
  // audio content, which is fine for parser tests.
  bool touch(const QString &path) {
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.flush();
  }
}

class TestFileParser : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();

  void emptyPlaylistFileReturnsNothing();
  void m3uIgnoresCommentsAndBlanks();
  void m3uPicksUpAbsoluteLocalFile();
  void m3uPicksUpRelativeLocalFile();
  void m3uSkipsNonExistentLocalFile();
  void m3uSkipsUnsupportedExtension();
  void httpEntryProducesStreamTrack();
  void plsStyleFileEntryProducesStreamTrack();

private:
  QTemporaryDir tempDir;
};

void TestFileParser::init() {
  tempDir.setAutoRemove(true);
  QVERIFY(tempDir.isValid());
}

void TestFileParser::cleanup() {
  // Nothing — QTemporaryDir cleans itself up on destruction at end of run.
}

void TestFileParser::emptyPlaylistFileReturnsNothing() {
  const QString pl = tempDir.filePath(QStringLiteral("empty.m3u"));
  QVERIFY(touch(pl));
  FileParser p{QDir(pl)};
  QCOMPARE(p.tracks_list().size(), 0);
}

void TestFileParser::m3uIgnoresCommentsAndBlanks() {
  const QString pl = tempDir.filePath(QStringLiteral("commented.m3u"));
  QVERIFY(writeFile(pl,
    "#EXTM3U\n"
    "\n"
    "#EXTINF:123,Artist - Title\n"
    "\n"));
  FileParser p{QDir(pl)};
  QCOMPARE(p.tracks_list().size(), 0);
}

void TestFileParser::m3uPicksUpAbsoluteLocalFile() {
  const QString audio = tempDir.filePath(QStringLiteral("song.flac"));
  QVERIFY(touch(audio));
  const QString pl = tempDir.filePath(QStringLiteral("abs.m3u"));
  QVERIFY(writeFile(pl, (audio + "\n").toUtf8()));

  FileParser p{QDir(pl)};
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.first().path(), audio);
}

void TestFileParser::m3uPicksUpRelativeLocalFile() {
  // FileParser resolves relative paths against the parent of the playlist file
  // (it calls QDir(path.absolutePath()).absolutePath() for current_dir()).
  QDir t(tempDir.path());
  QVERIFY(t.mkpath(QStringLiteral("nested")));
  const QString audio = t.filePath(QStringLiteral("nested/clip.mp3"));
  QVERIFY(touch(audio));
  const QString pl = t.filePath(QStringLiteral("rel.m3u"));
  QVERIFY(writeFile(pl, "nested/clip.mp3\n"));

  FileParser p{QDir(pl)};
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 1);
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("nested/clip.mp3")));
}

void TestFileParser::m3uSkipsNonExistentLocalFile() {
  const QString pl = tempDir.filePath(QStringLiteral("ghost.m3u"));
  QVERIFY(writeFile(pl,
    "/this/path/does/not/exist.flac\n"
    "also-missing.mp3\n"));
  FileParser p{QDir(pl)};
  QCOMPARE(p.tracks_list().size(), 0);
}

void TestFileParser::m3uSkipsUnsupportedExtension() {
  const QString notes = tempDir.filePath(QStringLiteral("notes.txt"));
  QVERIFY(touch(notes));
  const QString pl = tempDir.filePath(QStringLiteral("mixed.m3u"));
  QVERIFY(writeFile(pl, (notes + "\n").toUtf8()));
  FileParser p{QDir(pl)};
  QCOMPARE(p.tracks_list().size(), 0);
}

void TestFileParser::httpEntryProducesStreamTrack() {
  const QString pl = tempDir.filePath(QStringLiteral("http.m3u"));
  QVERIFY(writeFile(pl,
    "http://example.com/stream.mp3\n"
    "HTTPS://example.com/secure-stream\n"));
  FileParser p{QDir(pl)};
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 2);
  QVERIFY(tracks.at(0).isStream());
  QVERIFY(tracks.at(1).isStream());
  QCOMPARE(tracks.at(0).url(),
           QUrl(QStringLiteral("http://example.com/stream.mp3")));
}

void TestFileParser::plsStyleFileEntryProducesStreamTrack() {
  // "FileN=URL" → strip first 6 characters and treat as stream URL.
  const QString pl = tempDir.filePath(QStringLiteral("pls.m3u"));
  QVERIFY(writeFile(pl, "File1=http://radio.example/stream\n"));
  FileParser p{QDir(pl)};
  auto tracks = p.tracks_list();
  QCOMPARE(tracks.size(), 1);
  QVERIFY(tracks.first().isStream());
  QCOMPARE(tracks.first().url(),
           QUrl(QStringLiteral("http://radio.example/stream")));
}

QTEST_GUILESS_MAIN(TestFileParser)
#include "tst_fileparser.moc"
