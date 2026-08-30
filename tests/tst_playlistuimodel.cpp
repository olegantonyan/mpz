#include <QtTest>
#include <QAbstractItemModelTester>
#include <QApplication>
#include <QFont>
#include <QMimeData>
#include <QStyle>

#include "playlist_ui/columnsconfig.h"
#include "playlist_ui/playlistmodel.h"
#include "tracksmimedata.h"

namespace {
  Track mk(const QString &title, const QString &album = "album") {
    return Track("/music/" + title + ".mp3", 0, "artist", album, title, 1, 2000, 1000, 2, 320, 44100);
  }

  QMimeData *rowsMime(const QList<int> &rows) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << static_cast<qint32>(rows.size());
    for (int r : rows) {
      stream << static_cast<qint32>(r);
    }
    auto *mime = new QMimeData;
    mime->setData(QStringLiteral("application/x-mpz-playlist-tracks"), bytes);
    return mime;
  }

  QStringList titles(PlaylistUi::Model &model) {
    QStringList result;
    for (int i = 0; i < model.rowCount(); i++) {
      result << model.trackAt(i).title();
    }
    return result;
  }
}

class TestPlaylistUiModel : public QObject {
  Q_OBJECT
private slots:
  void init();
  void columnZeroIsTheStateGutter();
  void data_exposesStreamRoles();
  void data_boldsOnlyTheHighlightedTrack();
  void data_decoratesTheGutterPerHighlightState();
  void itemAt_isBoundsChecked();
  void indexOf_returnsInvalidForAnUnknownUid();
  void insertTracks_clampsThePosition();
  void remove_dropsTheGivenRows();
  void mimeData_isNullForAnEmptySelection();
  void mimeData_suggestsTheCommonAlbum();
  void dropMimeData_movesASingleRow();
  void dropMimeData_movesAContiguousBlock();
  void sortBy_reordersThroughTheSorter();
  void satisfiesTheModelContract();

private:
  PlaylistUi::ColumnsConfig columns;
  std::unique_ptr<PlaylistUi::Model> model;
  std::shared_ptr<Playlist::Playlist> playlist;

  void load(const QVector<Track> &tracks);
};

void TestPlaylistUiModel::init() {
  model = std::make_unique<PlaylistUi::Model>(qApp->style(), columns);
  playlist = std::make_shared<Playlist::Playlist>();
}

void TestPlaylistUiModel::load(const QVector<Track> &tracks) {
  playlist->load(tracks);
  model->setPlaylist(playlist);
}

void TestPlaylistUiModel::columnZeroIsTheStateGutter() {
  load({mk("one")});

  QCOMPARE(model->columnCount(), columns.count() + 1);
  QVERIFY(!model->headerData(0, Qt::Horizontal).isValid());
  QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), columns.field(1));
  QVERIFY(model->data(model->index(0, 0), Qt::DisplayRole).toString().isEmpty());
  QCOMPARE(model->data(model->index(0, 0), Qt::TextAlignmentRole).toInt(), int(Qt::AlignVCenter));
}

void TestPlaylistUiModel::data_exposesStreamRoles() {
  Track stream(QUrl("http://radio.example/stream"), "radio://x", "Station");
  load({mk("local"), stream});

  QVERIFY(!model->data(model->index(0, 1), PlaylistUi::Model::IsStreamRole).toBool());
  QVERIFY(model->data(model->index(1, 1), PlaylistUi::Model::IsStreamRole).toBool());
  QCOMPARE(model->data(model->index(1, 1), PlaylistUi::Model::StationNameRole).toString(),
           model->trackAt(1).stationName());
}

void TestPlaylistUiModel::data_boldsOnlyTheHighlightedTrack() {
  load({mk("one"), mk("two")});
  model->highlight(model->trackAt(1).uid(), PlaylistUi::Model::Playing);

  QVERIFY(!model->data(model->index(0, 1), Qt::FontRole).value<QFont>().bold());
  QVERIFY(model->data(model->index(1, 1), Qt::FontRole).value<QFont>().bold());
}

void TestPlaylistUiModel::data_decoratesTheGutterPerHighlightState() {
  load({mk("one")});
  const QModelIndex gutter = model->index(0, 0);

  QVERIFY(!model->data(gutter, Qt::DecorationRole).isValid());

  model->highlight(model->trackAt(0).uid(), PlaylistUi::Model::Playing);
  QVERIFY(model->data(gutter, Qt::DecorationRole).isValid());
  QVERIFY(!model->data(model->index(0, 1), Qt::DecorationRole).isValid());

  model->highlight(model->trackAt(0).uid(), PlaylistUi::Model::None);
  QVERIFY(!model->data(gutter, Qt::DecorationRole).isValid());
}

void TestPlaylistUiModel::itemAt_isBoundsChecked() {
  load({mk("one")});

  QCOMPARE(model->itemAt(model->index(0, 0)).title(), QString("one"));
  QVERIFY(!model->itemAt(model->buildIndex(5)).isValid());
  QVERIFY(!model->itemAt(model->buildIndex(-1)).isValid());
}

void TestPlaylistUiModel::indexOf_returnsInvalidForAnUnknownUid() {
  load({mk("one")});

  QCOMPARE(model->indexOf(model->trackAt(0).uid()).row(), 0);
  QVERIFY(!model->indexOf(123456).isValid());
}

void TestPlaylistUiModel::insertTracks_clampsThePosition() {
  load({mk("one"), mk("two")});
  QSignalSpy spy(model.get(), &PlaylistUi::Model::appendToPlaylistAsyncFinished);

  model->insertTracks({mk("head")}, -5);
  model->insertTracks({mk("tail")}, 99);

  // The playlist is updated in place; the view refresh is the controller's job.
  QCOMPARE(spy.count(), 2);
  model->reload();
  QCOMPARE(titles(*model), QStringList({"head", "one", "two", "tail"}));

  model->insertTracks({}, 0);
  QCOMPARE(spy.count(), 2);
}

void TestPlaylistUiModel::remove_dropsTheGivenRows() {
  load({mk("one"), mk("two"), mk("three")});

  model->remove({model->buildIndex(0), model->buildIndex(2)});

  QCOMPARE(titles(*model), QStringList({"two"}));
  QCOMPARE(playlist->tracks().size(), 1);
}

void TestPlaylistUiModel::mimeData_isNullForAnEmptySelection() {
  load({mk("one")});
  QCOMPARE(model->mimeData({}), nullptr);
  QCOMPARE(model->mimeData({QModelIndex()}), nullptr);
}

void TestPlaylistUiModel::mimeData_suggestsTheCommonAlbum() {
  load({mk("one", "same"), mk("two", "same"), mk("three", "other")});

  std::unique_ptr<QMimeData> same(model->mimeData({model->buildIndex(0), model->buildIndex(1)}));
  QCOMPARE(TracksMimeData::from(same.get())->suggestedName(), QString("same"));

  playlist->rename("fallback");
  std::unique_ptr<QMimeData> mixed(model->mimeData({model->buildIndex(0), model->buildIndex(2)}));
  QCOMPARE(TracksMimeData::from(mixed.get())->suggestedName(), QString("fallback"));
}

void TestPlaylistUiModel::dropMimeData_movesASingleRow() {
  load({mk("one"), mk("two"), mk("three")});
  std::unique_ptr<QMimeData> mime(rowsMime({0}));

  // Returns false on purpose so the view does not also remove the source row.
  QVERIFY(!model->dropMimeData(mime.get(), Qt::MoveAction, 3, 0, QModelIndex()));

  QCOMPARE(titles(*model), QStringList({"two", "three", "one"}));
  QCOMPARE(playlist->tracks().first().title(), QString("two"));
}

void TestPlaylistUiModel::dropMimeData_movesAContiguousBlock() {
  load({mk("one"), mk("two"), mk("three"), mk("four")});
  std::unique_ptr<QMimeData> mime(rowsMime({2, 3}));

  model->dropMimeData(mime.get(), Qt::MoveAction, 0, 0, QModelIndex());

  QCOMPARE(titles(*model), QStringList({"three", "four", "one", "two"}));
}

void TestPlaylistUiModel::sortBy_reordersThroughTheSorter() {
  load({mk("zzz"), mk("aaa")});

  model->sortBy("Title");

  QCOMPARE(titles(*model), QStringList({"aaa", "zzz"}));
}

void TestPlaylistUiModel::satisfiesTheModelContract() {
  QAbstractItemModelTester tester(model.get(), QAbstractItemModelTester::FailureReportingMode::QtTest);
  load({mk("one"), mk("two")});
  model->remove({model->buildIndex(0)});
  model->insertTracks({mk("three")}, 0);
}

QTEST_MAIN(TestPlaylistUiModel)
#include "tst_playlistuimodel.moc"
