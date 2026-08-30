#include "server.h"

#include "coverart/mpd.h"
#include "mpd_client/client.h"

#include <QSignalSpy>
#include <QtTest>

namespace {
  QStringList paths(const QVector<MpdClient::Entity> &entities, MpdClient::Entity::Type type) {
    QStringList result;
    for (const auto &e : entities) {
      if (e.type() == type) {
        result << e.path();
      }
    }
    return result;
  }

  QStringList filepaths(const QVector<MpdClient::Song> &songs) {
    QStringList result;
    for (const auto &s : songs) {
      result << s.filepath;
    }
    return result;
  }
}

// Database, stored playlist and cover-art commands against a real server. The
// raw protocol socket on the fixture is the oracle and the second client.
class TestMpdCommands : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();

  void lsDir_listsTheRootDirectories();
  void lsDir_listsSongsInsideADirectory();
  void lsDir_isEmptyForAMissingPath();
  void lsDirsSongs_flattensSeveralDirectories();
  void lsDirsSongs_readsTagsIntoSongFields();
  void createPlaylist_storesTheGivenSongsInOrder();
  void createPlaylist_overwritesAnExistingPlaylist();
  void playlists_listsWhatWasCreated();
  void lsPlaylistSongs_returnsThePlaylistContent();
  void appendSongsToPlaylist_addsToTheEnd();
  void removeSongsFromPlaylist_dropsTheGivenIndices();
  void removeSongsFromPlaylist_dropsEveryIndexOfAMultiSelection();
  void renamePlaylist_changesTheName();
  void renamePlaylist_failsForAMissingPlaylist();
  void removePlaylist_deletesIt();
  void removePlaylist_failsForAMissingPlaylist();
  void playlistNamesWithADivisionSlashRoundTrip();
  void updateDb_emitsDatabaseUpdatedAndPicksUpANewFile();
  void outputs_listsTheTwoConfiguredOutputs();
  void audioOutputChangeFromAnotherClientEmitsAudioOutputChanged();
  void playlistChangeFromAnotherClientEmitsPlaylistUpdated();
  void albumArt_returnsTheCoverNextToTheSong();
  void albumArt_isEmptyWhenThereIsNoCoverFile();
  void readPicture_returnsTheEmbeddedFlacPicture();
  void readPicture_isEmptyForAFileWithoutPictures();
  void coverArtMpd_writesTheFetchedImageToACachedTempFile();
  void coverArtMpd_fallsBackToReadPictureWhenThereIsNoCoverFile();

private:
  MpdTest::Server server;
  std::unique_ptr<MpdClient::Client> client;
};

void TestMpdCommands::initTestCase() {
  if (!MpdTest::Server::installed()) {
    QSKIP("mpd is not installed");
  }
  MpdTest::registerMetaTypes();
  QVERIFY2(server.start(), qPrintable(server.failReason()));

  client = std::make_unique<MpdClient::Client>();
  client->openConnection(server.url());
  QVERIFY(client->ping());
}

void TestMpdCommands::cleanup() {
  server.resetState();
  // Queued cross-thread signals must land before the next case's spy exists.
  QTest::qWait(50);
}

void TestMpdCommands::lsDir_listsTheRootDirectories() {
  // Only the directories are asserted: the tree model ignores ENTITY_PLAYLIST,
  // and whether lsinfo reports stored playlists at the root varies by mpd version.
  const QStringList dirs = paths(client->lsDir(""), MpdClient::Entity::ENTITY_DIR);

  QCOMPARE(dirs.size(), 4);
  for (const QString &expected : {"wav", "tagged", "covered", "embedded"}) {
    QVERIFY2(dirs.contains(expected), qPrintable(dirs.join(", ")));
  }
}

void TestMpdCommands::lsDir_listsSongsInsideADirectory() {
  const QStringList songs = paths(client->lsDir("tagged"), MpdClient::Entity::ENTITY_SONG);

  QCOMPARE(songs.size(), 2);
  QVERIFY(songs.contains("tagged/one.mp3"));
  QVERIFY(songs.contains("tagged/two.flac"));
}

void TestMpdCommands::lsDir_isEmptyForAMissingPath() {
  QVERIFY(client->lsDir("no/such/dir").isEmpty());
}

void TestMpdCommands::lsDirsSongs_flattensSeveralDirectories() {
  const QStringList songs = filepaths(client->lsDirsSongs({"tagged", "wav"}));

  QCOMPARE(songs.size(), 4);
  QVERIFY(songs.contains("tagged/one.mp3"));
  QVERIFY(songs.contains("wav/long_a.wav"));
}

void TestMpdCommands::lsDirsSongs_readsTagsIntoSongFields() {
  const auto songs = client->lsDirsSongs({"tagged"});
  QCOMPARE(songs.size(), 2);

  MpdClient::Song one;
  for (const auto &s : songs) {
    if (s.filepath == "tagged/one.mp3") {
      one = s;
    }
  }
  QCOMPARE(one.title, QString("One"));
  QCOMPARE(one.artist, QString("Tagged Artist"));
  QCOMPARE(one.album, QString("Tagged Album"));
  QCOMPARE(one.trackNumber, 1);
  QCOMPARE(one.date, QString("1999"));
  QVERIFY(one.duration >= 0);
}

void TestMpdCommands::createPlaylist_storesTheGivenSongsInOrder() {
  QVERIFY(client->createPlaylist({"tagged/two.flac", "tagged/one.mp3"}, "ordered"));

  QCOMPARE(filepaths(client->lsPlaylistSongs("ordered")),
           QStringList({"tagged/two.flac", "tagged/one.mp3"}));
}

void TestMpdCommands::createPlaylist_overwritesAnExistingPlaylist() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3", "tagged/two.flac"}, "reused"));
  QVERIFY(client->createPlaylist({"wav/long_a.wav"}, "reused"));

  QCOMPARE(filepaths(client->lsPlaylistSongs("reused")), QStringList({"wav/long_a.wav"}));
}

void TestMpdCommands::playlists_listsWhatWasCreated() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "first"));
  QVERIFY(client->createPlaylist({"tagged/two.flac"}, "second"));

  const QStringList names = paths(client->playlists(), MpdClient::Entity::ENTITY_PLAYLIST);
  QCOMPARE(names.size(), 2);
  QVERIFY(names.contains("first"));
  QVERIFY(names.contains("second"));
}

void TestMpdCommands::lsPlaylistSongs_returnsThePlaylistContent() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "content"));

  const auto songs = client->lsPlaylistSongs("content");
  QCOMPARE(songs.size(), 1);
  QCOMPARE(songs.first().filepath, QString("tagged/one.mp3"));
  QCOMPARE(songs.first().title, QString("One"));
}

void TestMpdCommands::appendSongsToPlaylist_addsToTheEnd() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "growing"));
  QVERIFY(client->appendSongsToPlaylist({"tagged/two.flac", "wav/long_a.wav"}, "growing"));

  QCOMPARE(filepaths(client->lsPlaylistSongs("growing")),
           QStringList({"tagged/one.mp3", "tagged/two.flac", "wav/long_a.wav"}));
}

void TestMpdCommands::removeSongsFromPlaylist_dropsTheGivenIndices() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3", "tagged/two.flac", "wav/long_a.wav"}, "shrinking"));
  QVERIFY(client->removeSongsFromPlaylist({1}, "shrinking"));

  QCOMPARE(filepaths(client->lsPlaylistSongs("shrinking")),
           QStringList({"tagged/one.mp3", "wav/long_a.wav"}));
}

void TestMpdCommands::removeSongsFromPlaylist_dropsEveryIndexOfAMultiSelection() {
  // The indices all refer to the playlist as it was before the call, so removing
  // several at once must not let an earlier delete shift the later ones.
  QVERIFY(client->createPlaylist(
    {"tagged/one.mp3", "tagged/two.flac", "wav/long_a.wav", "wav/long_b.wav"}, "multi"));
  QVERIFY(client->removeSongsFromPlaylist({0, 2}, "multi"));

  QCOMPARE(filepaths(client->lsPlaylistSongs("multi")),
           QStringList({"tagged/two.flac", "wav/long_b.wav"}));
}

void TestMpdCommands::renamePlaylist_changesTheName() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "before"));
  QVERIFY(client->renamePlaylist("before", "after"));

  const QStringList names = paths(client->playlists(), MpdClient::Entity::ENTITY_PLAYLIST);
  QVERIFY(!names.contains("before"));
  QVERIFY(names.contains("after"));
}

void TestMpdCommands::renamePlaylist_failsForAMissingPlaylist() {
  QVERIFY(!client->renamePlaylist("ghost", "still_a_ghost"));
}

void TestMpdCommands::removePlaylist_deletesIt() {
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "doomed"));
  QVERIFY(client->removePlaylist("doomed"));

  QVERIFY(client->playlists().isEmpty());
}

void TestMpdCommands::removePlaylist_failsForAMissingPlaylist() {
  QVERIFY(!client->removePlaylist("never_existed"));
}

void TestMpdCommands::playlistNamesWithADivisionSlashRoundTrip() {
  // The playlists model swaps '/' for U+2215 because mpd names cannot nest.
  const QString name = QString("artist%1album").arg(QChar(0x2215));
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, name));

  QVERIFY(paths(client->playlists(), MpdClient::Entity::ENTITY_PLAYLIST).contains(name));
  QCOMPARE(filepaths(client->lsPlaylistSongs(name)), QStringList({"tagged/one.mp3"}));
}

void TestMpdCommands::updateDb_emitsDatabaseUpdatedAndPicksUpANewFile() {
  QSignalSpy updated(client.get(), &MpdClient::Client::databaseUpdated);
  QVERIFY(MpdTest::writeWav(server.musicDir() + "/wav/added.wav", 1));

  client->updateDb();

  QTRY_VERIFY_WITH_TIMEOUT(updated.count() >= 1, 30000);
  QTRY_VERIFY(filepaths(client->lsDirsSongs({"wav"})).contains("wav/added.wav"));

  QVERIFY(QFile::remove(server.musicDir() + "/wav/added.wav"));
  client->updateDb();
  QTRY_VERIFY_WITH_TIMEOUT(!filepaths(client->lsDirsSongs({"wav"})).contains("wav/added.wav"), 30000);
}

void TestMpdCommands::outputs_listsTheTwoConfiguredOutputs() {
  const auto outputs = client->outputs();

  QCOMPARE(outputs.size(), 2);
  QCOMPARE(outputs.at(0).id(), 0);
  QCOMPARE(outputs.at(0).name(), QString("mpz test out"));
  QCOMPARE(outputs.at(1).name(), QString("mpz test out 2"));
  QVERIFY(outputs.at(0).isEnabled());
  QVERIFY(outputs.at(0).isValid());
}

void TestMpdCommands::audioOutputChangeFromAnotherClientEmitsAudioOutputChanged() {
  QSignalSpy changed(client.get(), &MpdClient::Client::audioOutputChanged);

  server.command("disableoutput 1");

  QTRY_VERIFY(changed.count() >= 1);
  const auto outputs = client->outputs();
  QCOMPARE(outputs.at(1).state(), MpdClient::Output::STATE_DISABLED);
  QVERIFY(!outputs.at(1).isEnabled());
}

void TestMpdCommands::playlistChangeFromAnotherClientEmitsPlaylistUpdated() {
  QSignalSpy updated(client.get(), &MpdClient::Client::playlistUpdated);

  server.command("save \"from_elsewhere\"");

  QTRY_VERIFY(updated.count() >= 1);
  QVERIFY(paths(client->playlists(), MpdClient::Entity::ENTITY_PLAYLIST).contains("from_elsewhere"));
}

void TestMpdCommands::albumArt_returnsTheCoverNextToTheSong() {
  const QByteArray art = client->albumArt("covered/withcover.mp3");

  QVERIFY(!art.isEmpty());
  QCOMPARE(art, MpdTest::pngBytes());
}

void TestMpdCommands::albumArt_isEmptyWhenThereIsNoCoverFile() {
  QVERIFY(client->albumArt("tagged/one.mp3").isEmpty());
  // The error has to be cleared, or every later command on that socket fails.
  QVERIFY(client->ping());
}

void TestMpdCommands::readPicture_returnsTheEmbeddedFlacPicture() {
  const QByteArray raw = client->readPicture("embedded/embedded.flac");

  QVERIFY(!raw.isEmpty());
  QVERIFY(raw.contains(MpdTest::pngBytes()));
}

void TestMpdCommands::readPicture_isEmptyForAFileWithoutPictures() {
  QVERIFY(client->readPicture("wav/long_a.wav").isEmpty());
  QVERIFY(client->ping());
}

void TestMpdCommands::coverArtMpd_writesTheFetchedImageToACachedTempFile() {
  CoverArt::Mpd covers(*client);

  const QString first = covers.get("covered/withcover.mp3");
  QVERIFY(!first.isEmpty());
  QVERIFY(QFile::exists(first));

  QFile written(first);
  QVERIFY(written.open(QIODevice::ReadOnly));
  QCOMPARE(written.readAll(), MpdTest::pngBytes());

  // The same image must come back from the cache rather than a second tempfile.
  QCOMPARE(covers.get("covered/withcover.mp3"), first);
}

void TestMpdCommands::coverArtMpd_fallsBackToReadPictureWhenThereIsNoCoverFile() {
  CoverArt::Mpd covers(*client);

  const QString path = covers.get("embedded/embedded.flac");

  QVERIFY(!path.isEmpty());
  QFile written(path);
  QVERIFY(written.open(QIODevice::ReadOnly));
  QCOMPARE(written.readAll(), MpdTest::pngBytes());
}

QTEST_MAIN(TestMpdCommands)

#include "tst_mpdcommands.moc"
