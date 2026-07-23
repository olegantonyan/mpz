#include <QtTest>

#include "playlist/playlist.h"
#include "track.h"

using Model = Playlist::Playlist;

namespace {
  Track mk(const QString &filepath,
           const QString &artist,
           const QString &album,
           const QString &title,
           quint16 tracknum,
           quint16 year) {
    return Track(filepath, 0, artist, album, title, tracknum, year, 0, 0, 0, 0);
  }
}

class TestPlaylist : public QObject {
  Q_OBJECT
private slots:
  void renameShortPassthrough();
  void renameTruncatesLongNames();
  void toM3UMixesPathsAndStreams();
  void appendWithSortOrdersByCriteria();
  void appendWithoutSortKeepsInsertionOrder();
  void loadReplacesContents();
  void trackLookupHitAndMiss();
  void removeTrackShrinks();
  void randomRoundTrip();
  void uidNonZeroAndStable();
};

void TestPlaylist::renameShortPassthrough() {
  Model pl;
  QCOMPARE(pl.rename(QStringLiteral("My Playlist")), QStringLiteral("My Playlist"));
  QCOMPARE(pl.name(), QStringLiteral("My Playlist"));
}

void TestPlaylist::renameTruncatesLongNames() {
  Model pl;
  const QString long_name(120, QChar('x'));
  const QString result = pl.rename(long_name);
  QCOMPARE(result.length(), 69);
  QVERIFY(result.endsWith(QStringLiteral("...")));
  QCOMPARE(pl.name(), result);
}

void TestPlaylist::toM3UMixesPathsAndStreams() {
  Model pl;
  QVector<Track> tracks;
  tracks << mk(QStringLiteral("/music/a.flac"), {}, {}, {}, 0, 0);
  tracks << Track(QUrl(QStringLiteral("http://stream.example/radio")), QStringLiteral("radio"));
  tracks << mk(QStringLiteral("/music/b.flac"), {}, {}, {}, 0, 0);
  pl.load(tracks);

  const QByteArray m3u = pl.toM3U();
  const QList<QByteArray> lines = m3u.split('\n');
  QCOMPARE(lines.size(), 3);
  QCOMPARE(lines.at(0), QByteArray("/music/a.flac"));
  QCOMPARE(lines.at(1), QByteArray("http://stream.example/radio"));
  QCOMPARE(lines.at(2), QByteArray("/music/b.flac"));
}

void TestPlaylist::appendWithSortOrdersByCriteria() {
  Model pl;
  QVector<Track> tracks;
  tracks << mk(QStringLiteral("/a.flac"), {}, QStringLiteral("Z"), {}, 0, 2010);
  tracks << mk(QStringLiteral("/b.flac"), {}, QStringLiteral("A"), {}, 0, 1990);
  tracks << mk(QStringLiteral("/c.flac"), {}, QStringLiteral("M"), {}, 0, 2000);
  pl.append(tracks, true);

  const QVector<Track> out = pl.tracks();
  QCOMPARE(out.size(), 3);
  QCOMPARE(out.at(0).year(), quint16{1990});
  QCOMPARE(out.at(1).year(), quint16{2000});
  QCOMPARE(out.at(2).year(), quint16{2010});
}

void TestPlaylist::appendWithoutSortKeepsInsertionOrder() {
  Model pl;
  QVector<Track> tracks;
  tracks << mk(QStringLiteral("/a.flac"), {}, {}, {}, 0, 2010);
  tracks << mk(QStringLiteral("/b.flac"), {}, {}, {}, 0, 1990);
  pl.append(tracks, false);

  const QVector<Track> out = pl.tracks();
  QCOMPARE(out.size(), 2);
  QCOMPARE(out.at(0).year(), quint16{2010});
  QCOMPARE(out.at(1).year(), quint16{1990});
}

void TestPlaylist::loadReplacesContents() {
  Model pl;
  QVector<Track> first;
  first << mk(QStringLiteral("/a.flac"), {}, {}, {}, 0, 0);
  pl.load(first);

  QVector<Track> second;
  second << mk(QStringLiteral("/b.flac"), {}, {}, {}, 0, 0);
  second << mk(QStringLiteral("/c.flac"), {}, {}, {}, 0, 0);
  pl.load(second);

  QCOMPARE(pl.tracks().size(), 2);
  QCOMPARE(pl.tracks().at(0).path(), QStringLiteral("/b.flac"));
}

void TestPlaylist::trackLookupHitAndMiss() {
  Model pl;
  Track a = mk(QStringLiteral("/a.flac"), {}, {}, {}, 0, 0);
  Track b = mk(QStringLiteral("/b.flac"), {}, {}, {}, 0, 0);
  QVector<Track> tracks;
  tracks << a << b;
  pl.load(tracks);

  QVERIFY(pl.hasTrack(b.uid()));
  QCOMPARE(pl.trackIndex(b.uid()), 1);
  QCOMPARE(pl.trackBy(b.uid()).path(), QStringLiteral("/b.flac"));

  const quint64 absent = a.uid() ^ b.uid() ^ quint64{0x1};
  QVERIFY(!pl.hasTrack(absent));
  QCOMPARE(pl.trackIndex(absent), -1);
  QCOMPARE(pl.trackBy(absent).uid(), quint64{0});
}

void TestPlaylist::removeTrackShrinks() {
  Model pl;
  QVector<Track> tracks;
  tracks << mk(QStringLiteral("/a.flac"), {}, {}, {}, 0, 0);
  tracks << mk(QStringLiteral("/b.flac"), {}, {}, {}, 0, 0);
  tracks << mk(QStringLiteral("/c.flac"), {}, {}, {}, 0, 0);
  pl.load(tracks);

  pl.removeTrack(1);
  const QVector<Track> out = pl.tracks();
  QCOMPARE(out.size(), 2);
  QCOMPARE(out.at(0).path(), QStringLiteral("/a.flac"));
  QCOMPARE(out.at(1).path(), QStringLiteral("/c.flac"));
}

void TestPlaylist::randomRoundTrip() {
  Model pl;
  QCOMPARE(pl.random(), Model::PlaylistRandom::None);
  pl.setRandom(Model::PlaylistRandom::Random);
  QCOMPARE(pl.random(), Model::PlaylistRandom::Random);
  pl.setRandom(Model::PlaylistRandom::SequentialNoLoop);
  QCOMPARE(pl.random(), Model::PlaylistRandom::SequentialNoLoop);
}

void TestPlaylist::uidNonZeroAndStable() {
  Model pl;
  const quint64 uid = pl.uid();
  QVERIFY(uid != 0);
  QCOMPARE(pl.uid(), uid);
}

QTEST_GUILESS_MAIN(TestPlaylist)
#include "tst_playlist.moc"
