#include <QtTest>
#include <QMenu>
#include <QTemporaryDir>
#include <QToolButton>

#include "config/global.h"
#include "playlist/sorter.h"
#include "sort_ui/sortmenu.h"

class TestSortPresets : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();
  void knownCriteriaOrderTracks();
  void unknownCriterionOrdersNothing();
  void defaultCriteriaAreAllUnderstood();
  void standardPresetsAreAllUnderstood();
  void attachToMenu_seedsAnEmptyConfig();
  void attachToMenu_repairsTheLegacyAristTypo();
  void attachToMenu_triggersTheSelectedCriteria();

private:
  QStringList presetCriteria(QMenu &menu) const;

  // Differ in every sortable field, so a criterion the sorter understands orders them strictly and one it does
  // not leaves them equal. Track::dir() canonicalizes, so DIRECTORY only sorts when the paths really exist.
  Track make(const QString &name, quint16 tracknum, quint16 year);
  bool orders(const QString &criterion);

  QTemporaryDir dir;
  Track low, high;
};

void TestSortPresets::initTestCase() {
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
  low = make("aaa", 1, 1990);
  high = make("zzz", 9, 2020);
}

void TestSortPresets::init() {
  QFile::remove(dir.filePath("global.yml"));
}

void TestSortPresets::cleanupTestCase() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

Track TestSortPresets::make(const QString &name, quint16 tracknum, quint16 year) {
  const QString subdir = dir.filePath(name);
  Q_ASSERT(QDir().mkpath(subdir));
  const QString path = subdir + "/" + name + ".mp3";
  QFile f(path);
  Q_ASSERT(f.open(QIODevice::WriteOnly));
  f.write("x");
  f.close();

  Track t(path, 0, name, name, name, tracknum, year, 1000, 2, 320, 44100);
  t.setAlbumArtist(name);
  t.setDiscNumber(QString::number(tracknum));
  return t;
}

bool TestSortPresets::orders(const QString &criterion) {
  const Playlist::Sorter sorter(criterion);
  return sorter.condition(low, high) != sorter.condition(high, low);
}

void TestSortPresets::knownCriteriaOrderTracks() {
  for (const QString &attr : {"Artist", "AlbumArtist", "Album", "Year", "TrackNumber",
                              "DiscNumber", "Filename", "Title", "Directory"}) {
    QVERIFY2(orders(attr), qPrintable(attr));
    QVERIFY2(orders("-" + attr), qPrintable("-" + attr));
  }
}

void TestSortPresets::unknownCriterionOrdersNothing() {
  QVERIFY(!orders("Nonsense"));
}

void TestSortPresets::defaultCriteriaAreAllUnderstood() {
  for (const QString &attr : Playlist::Sorter::defaultCriteria().split("/")) {
    QVERIFY2(orders(attr.simplified()), qPrintable(attr));
  }
}

void TestSortPresets::standardPresetsAreAllUnderstood() {
  for (const auto &preset : SortUi::SortMenu::standardPresets()) {
    for (const QString &attr : preset.second.split("/")) {
      QVERIFY2(orders(attr.simplified()),
               qPrintable(QString("preset \"%1\" has an unknown criterion \"%2\"")
                            .arg(preset.second, attr.simplified())));
    }
  }
}

QStringList TestSortPresets::presetCriteria(QMenu &menu) const {
  QStringList result;
  const auto actions = menu.actions();
  // Default, separator, presets..., separator, Edit presets
  for (int i = 2; i < actions.size() - 2; i++) {
    result << actions.at(i)->data().toString();
  }
  return result;
}

void TestSortPresets::attachToMenu_seedsAnEmptyConfig() {
  Config::Global global;
  QVERIFY(global.sortPresets().isEmpty());
  QToolButton button;
  SortUi::SortMenu sort_menu(&button, global);
  QMenu menu;

  sort_menu.attachToMenu(&menu);

  QCOMPARE(menu.actions().first()->data().toString(), Playlist::Sorter::defaultCriteria());
  QCOMPARE(menu.actions().last()->text(), QString("Edit presets"));
  QCOMPARE(presetCriteria(menu).size(), SortUi::SortMenu::standardPresets().size());
  QCOMPARE(global.sortPresets().size(), SortUi::SortMenu::standardPresets().size());
}

void TestSortPresets::attachToMenu_repairsTheLegacyAristTypo() {
  Config::Global global;
  QList<SortingPreset> stored{SortingPreset("", "Arist / Album"), SortingPreset("", "Title")};
  QVERIFY(global.saveSortPresets(stored));
  QToolButton button;
  SortUi::SortMenu sort_menu(&button, global);
  QMenu menu;

  sort_menu.attachToMenu(&menu);

  QCOMPARE(presetCriteria(menu), QStringList({"Artist / Album", "Title"}));
  QCOMPARE(global.sortPresets().first().second, QString("Artist / Album"));
}

void TestSortPresets::attachToMenu_triggersTheSelectedCriteria() {
  Config::Global global;
  QToolButton button;
  SortUi::SortMenu sort_menu(&button, global);
  QMenu menu;
  sort_menu.attachToMenu(&menu);
  QSignalSpy spy(&sort_menu, &SortUi::SortMenu::triggered);

  menu.actions().first()->trigger();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toString(), Playlist::Sorter::defaultCriteria());
}

QTEST_MAIN(TestSortPresets)
#include "tst_sortpresets.moc"
