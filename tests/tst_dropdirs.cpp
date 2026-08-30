#include <QtTest>
#include <QMimeData>
#include <QTemporaryDir>

#include "dropdirs.h"

namespace {
  QMimeData *urlsMime(const QStringList &paths) {
    auto *mime = new QMimeData;
    QList<QUrl> urls;
    for (const auto &p : paths) {
      urls << (p.startsWith("http") ? QUrl(p) : QUrl::fromLocalFile(p));
    }
    mime->setUrls(urls);
    return mime;
  }
}

class TestDropDirs : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void droppedDirs_ignoresNullAndUrllessMime();
  void droppedDirs_skipsNonLocalUrls();
  void droppedDirs_takesDirsAndSupportedFilesOnly();
  void commonParentDir_sharedParentForSiblings();
  void commonParentDir_emptyWhenParentsDiffer();
  void libraryRoot_prefersTheExplicitPayload();

private:
  QTemporaryDir dir;
  QString album, other, song, note;
};

void TestDropDirs::initTestCase() {
  QVERIFY(dir.isValid());
  album = dir.filePath("album");
  other = dir.filePath("other");
  QVERIFY(QDir().mkpath(album));
  QVERIFY(QDir().mkpath(other));

  song = album + "/song.mp3";
  note = album + "/notes.txt";
  for (const QString &path : {song, note}) {
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x");
  }
}

void TestDropDirs::droppedDirs_ignoresNullAndUrllessMime() {
  QVERIFY(DropUtil::droppedDirs(nullptr).isEmpty());
  QMimeData empty;
  QVERIFY(DropUtil::droppedDirs(&empty).isEmpty());
}

void TestDropDirs::droppedDirs_skipsNonLocalUrls() {
  std::unique_ptr<QMimeData> mime(urlsMime({"http://example.com/stream", album}));
  const auto dirs = DropUtil::droppedDirs(mime.get());
  QCOMPARE(dirs.size(), 1);
  QCOMPARE(dirs.first().absolutePath(), QDir(album).absolutePath());
}

void TestDropDirs::droppedDirs_takesDirsAndSupportedFilesOnly() {
  std::unique_ptr<QMimeData> mime(urlsMime({album, song, note}));
  const auto dirs = DropUtil::droppedDirs(mime.get());
  QCOMPARE(dirs.size(), 2);
  QCOMPARE(dirs.at(0).absolutePath(), QDir(album).absolutePath());
  QCOMPARE(dirs.at(1).absolutePath(), QDir(song).absolutePath());
}

void TestDropDirs::commonParentDir_sharedParentForSiblings() {
  QCOMPARE(DropUtil::commonParentDir({QDir(album), QDir(other)}), QDir(dir.path()).absolutePath());
  QCOMPARE(DropUtil::commonParentDir({QDir(album)}), QDir(dir.path()).absolutePath());
  QVERIFY(DropUtil::commonParentDir({}).isEmpty());
}

void TestDropDirs::commonParentDir_emptyWhenParentsDiffer() {
  QVERIFY(DropUtil::commonParentDir({QDir(album), QDir(song)}).isEmpty());
}

void TestDropDirs::libraryRoot_prefersTheExplicitPayload() {
  std::unique_ptr<QMimeData> mime(urlsMime({album}));
  const auto dirs = DropUtil::droppedDirs(mime.get());
  QCOMPARE(DropUtil::libraryRoot(mime.get(), dirs), QDir(dir.path()).absolutePath());

  mime->setData(DropUtil::libraryRootFormat, "/music");
  QCOMPARE(DropUtil::libraryRoot(mime.get(), dirs), QString("/music"));
}

QTEST_GUILESS_MAIN(TestDropDirs)
#include "tst_dropdirs.moc"
