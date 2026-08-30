#include <QtTest>
#include <QTemporaryDir>

#include "config/local.h"
#include "playback_log_ui/playbackloguimodel.h"

class TestPlaybackLogModel : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void append_putsTheNewestFirst();
  void append_evictsTheOldestAtMaxSize();
  void data_formatsTimeAndText();
  void itemsToCsv_joinsRowsInViewOrder();
  void incrementPlayTime_reportsBothCountersAndPersists();

private:
  QTemporaryDir dir;
};

void TestPlaybackLogModel::init() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestPlaybackLogModel::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestPlaybackLogModel::append_putsTheNewestFirst() {
  Config::Local local;
  PlaybackLogUi::Model model(local, 10);
  QSignalSpy spy(&model, &PlaybackLogUi::Model::changed);

  model.append(PlaybackLogUi::Item(1, "first"));
  model.append(PlaybackLogUi::Item(2, "second"));

  QCOMPARE(model.rowCount(), 2);
  QCOMPARE(model.columnCount(), 2);
  QCOMPARE(model.last().text, QString("second"));
  QCOMPARE(model.itemAt(model.index(0, 0)).track_uid, 2u);
  QCOMPARE(spy.count(), 2);
}

void TestPlaybackLogModel::append_evictsTheOldestAtMaxSize() {
  Config::Local local;
  PlaybackLogUi::Model model(local, 2);

  model.append(PlaybackLogUi::Item(1, "oldest"));
  model.append(PlaybackLogUi::Item(2, "middle"));
  model.append(PlaybackLogUi::Item(3, "newest"));

  QCOMPARE(model.rowCount(), 2);
  QCOMPARE(model.itemAt(model.index(0, 0)).text, QString("newest"));
  QCOMPARE(model.itemAt(model.index(1, 0)).text, QString("middle"));
}

void TestPlaybackLogModel::data_formatsTimeAndText() {
  Config::Local local;
  PlaybackLogUi::Model model(local, 10);
  model.append(PlaybackLogUi::Item(1, "artist - title"));

  QCOMPARE(model.data(model.index(0, 1)).toString(), QString("artist - title"));
  QVERIFY(QTime::fromString(model.data(model.index(0, 0)).toString(), "HH:mm:ss").isValid());
  QVERIFY(!model.data(model.index(0, 2)).isValid());
  QVERIFY(!model.data(QModelIndex()).isValid());
}

void TestPlaybackLogModel::itemsToCsv_joinsRowsInViewOrder() {
  Config::Local local;
  PlaybackLogUi::Model model(local, 10);
  model.append(PlaybackLogUi::Item(1, "one"));
  model.append(PlaybackLogUi::Item(2, "two"));

  const auto rows = model.itemsToCsv().split("\n");
  QCOMPARE(rows.size(), 2);
  QVERIFY(rows.at(0).endsWith(",two"));
  QVERIFY(rows.at(1).endsWith(",one"));
}

void TestPlaybackLogModel::incrementPlayTime_reportsBothCountersAndPersists() {
  {
    Config::Local local;
    local.saveTotalPlaybackTime(100);
    PlaybackLogUi::Model model(local, 10);
    QSignalSpy total(&model, &PlaybackLogUi::Model::totalPlayTimeChanged);
    QSignalSpy session(&model, &PlaybackLogUi::Model::thisSessionPlayTimeChanged);

    model.incrementPlayTime(5);

    QCOMPARE(total.first().first().toInt(), 105);
    QCOMPARE(session.first().first().toInt(), 5);
  }

  Config::Local reopened;
  QCOMPARE(reopened.totalPlaybackTime(), 105);
}

QTEST_GUILESS_MAIN(TestPlaybackLogModel)
#include "tst_playbacklogmodel.moc"
