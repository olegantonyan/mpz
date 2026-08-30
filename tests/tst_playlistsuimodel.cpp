#include <QtTest>
#include <QAbstractItemModelTester>
#include <QMimeData>
#include <QTemporaryDir>

#include "config/local.h"
#include "playlists_ui/playlistsmodel.h"

namespace {
  class TestableModel : public PlaylistsUi::Model {
  public:
    using PlaylistsUi::Model::Model;
    using PlaylistsUi::Model::playlistNameBy;
  };

  std::shared_ptr<Playlist::Playlist> mkPlaylist(const QString &name) {
    auto pl = std::make_shared<Playlist::Playlist>();
    pl->rename(name);
    pl->load({Track("/music/" + name + ".mp3", 0, "artist", "album", name, 1, 2000, 1000, 2, 320, 44100)});
    return pl;
  }

  QMimeData *rowMime(int row) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << row;
    auto *mime = new QMimeData;
    mime->setData(QStringLiteral("application/x-mpz-playlist-row"), bytes);
    return mime;
  }

  QStringList names(PlaylistsUi::Model &model) {
    QStringList result;
    for (const auto &pl : model.itemList()) {
      result << pl->name();
    }
    return result;
  }
}

class TestPlaylistsUiModel : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void append_growsTheListAndPersists();
  void remove_ignoresAnOutOfRangeIndex();
  void lookups_findByUidAndByTrack();
  void itemIndex_isInvalidForAnUnknownPlaylist();
  void higlight_boldsOnlyTheHighlightedRow();
  void createPlaylistFromTracks_defersItsSignal();
  void createPlaylistFromTracks_ignoresAnEmptyVector();
  void dropMimeData_reordersAndPersists();
  void dropMimeData_isANoOpForANeighbourDrop();
  void currentPlaylistIndex_isClampedToTheList();
  void loadAsync_restoresPersistedPlaylists();
  void playlistNameBy_stripsTheLibraryPrefix();
  void satisfiesTheModelContract();

private:
  QTemporaryDir dir;
};

void TestPlaylistsUiModel::init() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestPlaylistsUiModel::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
  QFile::remove(dir.filePath("local.yml"));
}

void TestPlaylistsUiModel::append_growsTheListAndPersists() {
  Config::Local local;
  PlaylistsUi::Model model(local);

  const QModelIndex idx = model.append(mkPlaylist("rock"));

  QCOMPARE(idx.row(), 0);
  QCOMPARE(model.listSize(), 1);
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.data(idx, Qt::DisplayRole).toString(), QString("rock"));
  QCOMPARE(local.playlists().size(), 1);
}

void TestPlaylistsUiModel::remove_ignoresAnOutOfRangeIndex() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  model.append(mkPlaylist("rock"));

  model.remove(QModelIndex());
  model.remove(model.buildIndex(7));
  QCOMPARE(model.listSize(), 1);

  model.remove(model.buildIndex(0));
  QCOMPARE(model.listSize(), 0);
  QVERIFY(local.playlists().isEmpty());
}

void TestPlaylistsUiModel::lookups_findByUidAndByTrack() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  auto rock = mkPlaylist("rock");
  model.append(rock);
  model.append(mkPlaylist("jazz"));

  QCOMPARE(model.itemBy(rock->uid()), rock);
  QCOMPARE(model.itemByTrack(rock->tracks().first().uid()), rock);
  QCOMPARE(model.itemBy(123456), nullptr);
  QCOMPARE(model.itemByTrack(123456), nullptr);
  QCOMPARE(model.itemAt(model.buildIndex(9)), nullptr);
}

void TestPlaylistsUiModel::itemIndex_isInvalidForAnUnknownPlaylist() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  model.append(mkPlaylist("rock"));

  QVERIFY(!model.itemIndex(mkPlaylist("stranger")).isValid());
}

void TestPlaylistsUiModel::higlight_boldsOnlyTheHighlightedRow() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  model.append(mkPlaylist("rock"));
  auto jazz = mkPlaylist("jazz");
  model.append(jazz);

  model.higlight(jazz);
  QVERIFY(!model.data(model.buildIndex(0), Qt::FontRole).isValid());
  QVERIFY(model.data(model.buildIndex(1), Qt::FontRole).value<QFont>().bold());

  model.higlight(nullptr);
  QVERIFY(!model.data(model.buildIndex(1), Qt::FontRole).isValid());
}

void TestPlaylistsUiModel::createPlaylistFromTracks_defersItsSignal() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  QSignalSpy spy(&model, &PlaylistsUi::Model::createPlaylistAsyncFinished);

  model.createPlaylistFromTracks(mkPlaylist("radio")->tracks(), "radio");

  // Deferred on purpose: the caller is usually inside a view event handler.
  QCOMPARE(spy.count(), 0);
  QVERIFY(spy.wait());
  QCOMPARE(spy.count(), 1);
}

void TestPlaylistsUiModel::createPlaylistFromTracks_ignoresAnEmptyVector() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  QSignalSpy spy(&model, &PlaylistsUi::Model::createPlaylistAsyncFinished);

  model.createPlaylistFromTracks({}, "empty");

  QTest::qWait(20);
  QCOMPARE(spy.count(), 0);
}

void TestPlaylistsUiModel::dropMimeData_reordersAndPersists() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  model.append(mkPlaylist("one"));
  model.append(mkPlaylist("two"));
  model.append(mkPlaylist("three"));
  std::unique_ptr<QMimeData> mime(rowMime(0));

  // Returns false so the view does not also remove the source row.
  QVERIFY(!model.dropMimeData(mime.get(), Qt::MoveAction, 3, 0, QModelIndex()));

  QCOMPARE(names(model), QStringList({"two", "three", "one"}));
  QCOMPARE(local.playlists().first()->name(), QString("two"));
}

void TestPlaylistsUiModel::dropMimeData_isANoOpForANeighbourDrop() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  model.append(mkPlaylist("one"));
  model.append(mkPlaylist("two"));

  for (int row : {0, 1}) {
    std::unique_ptr<QMimeData> mime(rowMime(0));
    QVERIFY(!model.dropMimeData(mime.get(), Qt::MoveAction, row, 0, QModelIndex()));
  }
  std::unique_ptr<QMimeData> stale(rowMime(9));
  QVERIFY(!model.dropMimeData(stale.get(), Qt::MoveAction, 0, 0, QModelIndex()));

  QCOMPARE(names(model), QStringList({"one", "two"}));
}

void TestPlaylistsUiModel::currentPlaylistIndex_isClampedToTheList() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  model.append(mkPlaylist("one"));
  model.append(mkPlaylist("two"));

  model.saveCurrentPlaylistIndex(model.buildIndex(9));
  QCOMPARE(local.currentPlaylist(), 1);
  QCOMPARE(model.currentPlaylistIndex().row(), 1);
}

void TestPlaylistsUiModel::loadAsync_restoresPersistedPlaylists() {
  {
    Config::Local local;
    PlaylistsUi::Model model(local);
    model.append(mkPlaylist("rock"));
    local.sync();
  }

  Config::Local local;
  PlaylistsUi::Model model(local);
  QSignalSpy spy(&model, &PlaylistsUi::Model::asyncLoadFinished);

  model.loadAsync();

  QVERIFY(spy.wait());
  QCOMPARE(names(model), QStringList({"rock"}));
  QCOMPARE(model.itemList().first()->tracks().first().title(), QString("rock"));
}

void TestPlaylistsUiModel::playlistNameBy_stripsTheLibraryPrefix() {
  Config::Local local;
  TestableModel model(local);

  QCOMPARE(model.playlistNameBy(QDir("/music/rock"), "/music"), QString("rock"));
  QCOMPARE(model.playlistNameBy(QDir("/music/rock"), ""), QString("music/rock"));
  QCOMPARE(model.playlistNameBy(QDir("/music"), "/music"), QString(""));
  // The library name reappearing deeper in the path must not be stripped too.
  QCOMPARE(model.playlistNameBy(QDir("/music/rock/music"), "/music"), QString("rock/music"));
  // ...and a sibling that merely starts with the library name is not the library.
  QCOMPARE(model.playlistNameBy(QDir("/musicvideos/rock"), "/music"), QString("musicvideos/rock"));
  QCOMPARE(model.playlistNameBy(QDir("/music/rock"), "/music/"), QString("rock"));
}

void TestPlaylistsUiModel::satisfiesTheModelContract() {
  Config::Local local;
  PlaylistsUi::Model model(local);
  QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

  model.append(mkPlaylist("one"));
  model.append(mkPlaylist("two"));
  model.remove(model.buildIndex(0));
}

QTEST_MAIN(TestPlaylistsUiModel)
#include "tst_playlistsuimodel.moc"
