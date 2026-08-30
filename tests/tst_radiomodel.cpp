#include <QtTest>
#include <QAbstractItemModelTester>
#include <QTemporaryDir>

#include "config/global.h"
#include "directory_ui/directorymodel/radio.h"
#include "directory_ui/radiolibrary.h"
#include "radio/catalog.h"

using RadioModel = DirectoryUi::DirectoryModel::Radio;
namespace RadioRole = DirectoryUi::DirectoryModel::RadioRole;

namespace {
  ::Radio::Station station(const QString &id, const QString &name, const QString &group) {
    ::Radio::Station st;
    st.id = id;
    st.name = name;
    st.group = group;
    st.url = "http://radio.example/" + id;
    st.codec = "mp3";
    st.bitrate = 128;
    return st;
  }
}

class TestRadioModel : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();
  void fallsBackToTheBuiltinCatalog();
  void groupsStationsInFirstSeenOrder();
  void loadAsync_reportsTheRadioPath();
  void filePath_distinguishesGroupsFromStations();
  void displayName_prefixesTheGroup();
  void isStation_isFalseForRootAndGroups();
  void data_countsVisibleStationsWithPluralForms();
  void data_exposesTheRadioRoles();
  void tracksAt_expandsAGroupToItsVisibleStations();
  void filter_survivesARebuild();
  void satisfiesTheModelContract();

private:
  QTemporaryDir dir;

  void seed(const QVector<::Radio::Station> &stations);
  QModelIndex groupNamed(RadioModel &model, const QString &name) const;
};

void TestRadioModel::init() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
}

void TestRadioModel::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
  QFile::remove(dir.filePath("global.yml"));
}

void TestRadioModel::seed(const QVector<::Radio::Station> &stations) {
  Config::Global global;
  QVERIFY(global.saveRadioStations(stations));
  QVERIFY(global.sync());
}

QModelIndex TestRadioModel::groupNamed(RadioModel &model, const QString &name) const {
  for (int i = 0; i < model.rowCount(QModelIndex()); i++) {
    const QModelIndex idx = model.index(i, 0, QModelIndex());
    if (model.data(idx, Qt::DisplayRole).toString().startsWith(name)) {
      return idx;
    }
  }
  return QModelIndex();
}

void TestRadioModel::fallsBackToTheBuiltinCatalog() {
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  QVERIFY(model.rowCount(QModelIndex()) > 0);
  QVERIFY(!::Radio::Catalog::builtin().isEmpty());
}

void TestRadioModel::groupsStationsInFirstSeenOrder() {
  seed({station("a", "Alpha", "Rock"), station("b", "Beta", ""), station("c", "Gamma", "Rock")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  // One synthesized group plus the ungrouped station, group first.
  QCOMPARE(model.rowCount(QModelIndex()), 2);
  QCOMPARE(model.rowCount(model.index(0, 0, QModelIndex())), 2);
  QCOMPARE(model.data(model.index(1, 0, QModelIndex()), Qt::DisplayRole).toString(), QString("Beta"));
}

void TestRadioModel::loadAsync_reportsTheRadioPath() {
  Config::Global global;
  RadioModel model(global);
  QSignalSpy spy(&model, &RadioModel::directoryLoaded);

  model.loadAsync(QString());
  QCOMPARE(spy.first().first().toString(), DirectoryUi::radioLibraryPath());

  model.loadAsync("radio://x");
  QCOMPARE(spy.last().first().toString(), QString("radio://x"));
}

void TestRadioModel::filePath_distinguishesGroupsFromStations() {
  seed({station("a", "Alpha", "Rock")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  const QModelIndex group = model.index(0, 0, QModelIndex());
  QCOMPARE(model.filePath(group), QString("radio://group/Rock"));
  QCOMPARE(model.filePath(model.index(0, 0, group)), QString("radio://a"));
  QVERIFY(model.filePath(QModelIndex()).isEmpty());
}

void TestRadioModel::displayName_prefixesTheGroup() {
  seed({station("a", "Alpha", "Rock"), station("b", "Beta", "")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  const QModelIndex group = model.index(0, 0, QModelIndex());
  QCOMPARE(model.displayName(model.index(0, 0, group)), QString("Rock ∕ Alpha"));
  QCOMPARE(model.displayName(model.index(1, 0, QModelIndex())), QString("Beta"));
  QCOMPARE(model.displayName(group), QString("Rock"));
  QVERIFY(model.displayName(QModelIndex()).isEmpty());
}

void TestRadioModel::isStation_isFalseForRootAndGroups() {
  seed({station("a", "Alpha", "Rock")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  const QModelIndex group = model.index(0, 0, QModelIndex());
  QVERIFY(!model.isStation(QModelIndex()));
  QVERIFY(!model.isStation(group));
  QVERIFY(model.isStation(model.index(0, 0, group)));
}

void TestRadioModel::data_countsVisibleStationsWithPluralForms() {
  seed({station("a", "Alpha", "Rock"), station("c", "Gamma", "Rock"), station("d", "Delta", "Jazz")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  QCOMPARE(model.data(groupNamed(model, "Rock"), Qt::DisplayRole).toString(), QString("Rock  ·  2 stations"));
  QCOMPARE(model.data(groupNamed(model, "Jazz"), Qt::DisplayRole).toString(), QString("Jazz  ·  1 station"));
}

void TestRadioModel::data_exposesTheRadioRoles() {
  ::Radio::Station st = station("a", "Alpha", "Rock");
  st.homepage = "http://alpha.example";
  seed({st});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());

  const QModelIndex group = model.index(0, 0, QModelIndex());
  const QModelIndex leaf = model.index(0, 0, group);

  QVERIFY(model.data(leaf, RadioRole::IsStation).toBool());
  QVERIFY(!model.data(group, RadioRole::IsStation).toBool());
  QCOMPARE(model.data(leaf, RadioRole::StreamUrl).toString(), st.url);
  QCOMPARE(model.data(leaf, RadioRole::Homepage).toString(), st.homepage);
  QCOMPARE(model.data(leaf, RadioRole::Path).toString(), QString("radio://a"));
  QCOMPARE(model.data(leaf, RadioRole::Subtitle).toString(), st.subtitle());
  QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
}

void TestRadioModel::tracksAt_expandsAGroupToItsVisibleStations() {
  seed({station("a", "Alpha", "Rock"), station("c", "Gamma", "Rock")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());
  const QModelIndex group = model.index(0, 0, QModelIndex());

  auto tracks = model.tracksAt({group});
  QCOMPARE(tracks.size(), 2);
  QCOMPARE(tracks.first().title(), QString("Rock ∕ Alpha"));
  QCOMPARE(tracks.first().path(), QString("radio://a"));
  QVERIFY(tracks.first().isStream());

  QCOMPARE(model.tracksAt({model.index(0, 0, group)}).size(), 1);
  QVERIFY(model.tracksAt({QModelIndex()}).isEmpty());

  model.filter("Gamma");
  QCOMPARE(model.tracksAt({groupNamed(model, "Rock")}).size(), 1);
}

void TestRadioModel::filter_survivesARebuild() {
  seed({station("a", "Alpha", "Rock"), station("d", "Delta", "Jazz")});
  Config::Global global;
  RadioModel model(global);
  model.loadAsync(QString());
  QCOMPARE(model.rowCount(QModelIndex()), 2);

  model.filter("Alpha");
  QCOMPARE(model.rowCount(QModelIndex()), 1);

  model.loadAsync(QString());
  QCOMPARE(model.rowCount(QModelIndex()), 1);

  model.filter("");
  QCOMPARE(model.rowCount(QModelIndex()), 2);
}

void TestRadioModel::satisfiesTheModelContract() {
  seed({station("a", "Alpha", "Rock"), station("c", "Gamma", "Rock"), station("b", "Beta", "")});
  Config::Global global;
  RadioModel model(global);
  QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

  model.loadAsync(QString());
  model.filter("Alpha");
  model.filter("");
}

QTEST_MAIN(TestRadioModel)
#include "tst_radiomodel.moc"
