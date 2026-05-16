#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "playlist/loader.h"

using Playlist::Loader;

namespace {
  bool touch(const QString &path) {
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.flush();
  }

  bool writeFile(const QString &path, const QByteArray &content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    return f.write(content) == content.size();
  }
}

class TestLoader : public QObject {
  Q_OBJECT
private slots:
  void init();

  void supportedFileFormatsIncludesCommonAudio();
  void supportedPlaylistFileFormatsIsCanonical();

  void isSupportedFileExactExtensions();
  void isSupportedFileIsCaseInsensitive();
  void isSupportedFileRejectsUnknownExtension();
  void isSupportedFileRejectsEmptyString();
  void isSupportedFileAcceptsCueExtension();

  void singleAudioFileReturnsOneTrack();
  void directoryWithLooseAudioFilesReturnsAllOfThem();
  void playlistFilePathDelegatesToFileParser();

private:
  QTemporaryDir tempDir;
};

void TestLoader::init() {
  tempDir.setAutoRemove(true);
  QVERIFY(tempDir.isValid());
}

void TestLoader::supportedFileFormatsIncludesCommonAudio() {
  const QStringList &f = Loader::supportedFileFormats();
  for (const QString &ext : { QStringLiteral("mp3"),
                              QStringLiteral("flac"),
                              QStringLiteral("ogg"),
                              QStringLiteral("m4a"),
                              QStringLiteral("opus"),
                              QStringLiteral("wav"),
                              QStringLiteral("cue") }) {
    QVERIFY2(f.contains(ext), qPrintable(QStringLiteral("missing %1").arg(ext)));
  }
}

void TestLoader::supportedPlaylistFileFormatsIsCanonical() {
  const QStringList &f = Loader::supportedPlaylistFileFormats();
  QVERIFY(f.contains(QStringLiteral("m3u")));
  QVERIFY(f.contains(QStringLiteral("m3u8")));
  QVERIFY(f.contains(QStringLiteral("pls")));
}

void TestLoader::isSupportedFileExactExtensions() {
  QVERIFY(Loader::is_supported_file(QStringLiteral("song.mp3")));
  QVERIFY(Loader::is_supported_file(QStringLiteral("song.flac")));
  QVERIFY(Loader::is_supported_file(QStringLiteral("song.opus")));
  QVERIFY(Loader::is_supported_file(QStringLiteral("a/long/path/song.dsf")));
}

void TestLoader::isSupportedFileIsCaseInsensitive() {
  QVERIFY(Loader::is_supported_file(QStringLiteral("Song.MP3")));
  QVERIFY(Loader::is_supported_file(QStringLiteral("X.FlAc")));
  QVERIFY(Loader::is_supported_file(QStringLiteral("Y.CUE")));
}

void TestLoader::isSupportedFileRejectsUnknownExtension() {
  QVERIFY(!Loader::is_supported_file(QStringLiteral("readme.txt")));
  QVERIFY(!Loader::is_supported_file(QStringLiteral("cover.jpg")));
  QVERIFY(!Loader::is_supported_file(QStringLiteral("song")));  // no ext
}

void TestLoader::isSupportedFileRejectsEmptyString() {
  QVERIFY(!Loader::is_supported_file(QString()));
  QVERIFY(!Loader::is_supported_file(QStringLiteral("")));
}

void TestLoader::isSupportedFileAcceptsCueExtension() {
  QVERIFY(Loader::is_supported_file(QStringLiteral("disc.cue")));
}

void TestLoader::singleAudioFileReturnsOneTrack() {
  const QString p = tempDir.filePath(QStringLiteral("single.flac"));
  QVERIFY(touch(p));
  Loader loader{QDir(p)};
  auto tracks = loader.tracks();
  QCOMPARE(tracks.size(), 1);
  // Track path comes from QDir::absolutePath(), which returns the input for a
  // file-shaped QDir.
  QVERIFY(tracks.first().path().endsWith(QStringLiteral("single.flac")));
}

void TestLoader::directoryWithLooseAudioFilesReturnsAllOfThem() {
  QDir d(tempDir.path());
  QVERIFY(d.mkpath(QStringLiteral("library")));
  QVERIFY(touch(d.filePath(QStringLiteral("library/a.flac"))));
  QVERIFY(touch(d.filePath(QStringLiteral("library/b.mp3"))));
  QVERIFY(touch(d.filePath(QStringLiteral("library/c.opus"))));
  QVERIFY(touch(d.filePath(QStringLiteral("library/ignore.txt"))));

  Loader loader{QDir(d.filePath(QStringLiteral("library")))};
  auto tracks = loader.tracks();
  QCOMPARE(tracks.size(), 3);
}

void TestLoader::playlistFilePathDelegatesToFileParser() {
  // For a .m3u path Loader::tracks() routes to FileParser.
  const QString audio = tempDir.filePath(QStringLiteral("queue.flac"));
  QVERIFY(touch(audio));
  const QString pl = tempDir.filePath(QStringLiteral("queue.m3u"));
  QVERIFY(writeFile(pl, (audio + "\n").toUtf8()));

  Loader loader{QDir(pl)};
  QVERIFY(loader.is_playlist_file());
  auto tracks = loader.tracks();
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.first().path(), audio);
}

QTEST_GUILESS_MAIN(TestLoader)
#include "tst_loader.moc"
