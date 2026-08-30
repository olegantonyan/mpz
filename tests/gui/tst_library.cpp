#include "fixture.h"

#include "directory_ui/directorycontroller.h"
#include "directory_ui/directorysettings.h"
#include "directory_ui/radiolibrary.h"
#include "modusoperandi.h"
#include "slidingbanner.h"

#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>

// Adding a folder is not reachable here: it opens QFileDialog, so the "add"
// path is covered by seeding library_paths and asserting the UI picks it up.
class TestLibrary : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void seededLibraryPopulatesTheComboAndTree();
  void radioIsAlwaysTheLastComboEntry();
  void switchingLibraryPersistsTheSelection();
  void switchingToRadioActivatesTheRadioTree();
  void filteringHidesTopLevelMisses();
  void selectionCreatesAPlaylistWithTheLibraryRoot();
  void emptySelectionCreatesNothing();
  void settingsRemoveDropsTheSelectedPath();
  void settingsRefusesToRemoveRadio();
  void settingsMoveButtonsReorderAndBound();

private:
  GuiTest::ConfigDir config;
  SlidingBanner banner;
  QTreeView view;
  QLineEdit search;
  QComboBox libswitch;
  QToolButton libcfg;
  QToolButton libsort;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<DirectoryUi::Controller> library;
  QString rock, jazz;

  void build();
  // QFileSystemModel populates on its own thread, so rows arrive after the load signal.
  void awaitRows(int expected);
  QModelIndex childNamed(const QString &name) const;
};

void TestLibrary::initTestCase() {
  rock = config.path() + "/rock";
  jazz = config.path() + "/jazz";
  QVERIFY(config.init({rock, jazz}));
  QVERIFY(GuiTest::copyAudioFixtures(rock + "/album"));
  QVERIFY(QDir().mkpath(rock + "/live"));
  QVERIFY(QDir().mkpath(jazz));
}

void TestLibrary::init() {
  build();
}

void TestLibrary::cleanup() {
  library.reset();
  modus.reset();
  local.reset();
  global.reset();
  libswitch.clear();
  // Tests that switch libraries persist current_library; start every test on rock.
  QFile::remove(config.path() + "/local.yml");
  QVERIFY(config.init({rock, jazz}));
}

void TestLibrary::build() {
  // Config::Global, Config::Local and ModusOperandi are single-instance guarded.
  library.reset();
  modus.reset();
  local.reset();
  global.reset();
  libswitch.clear();

  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  library = std::make_unique<DirectoryUi::Controller>(&view, &search, &libswitch, &libcfg, &libsort,
                                                      *local, *global, *modus);
  QTRY_VERIFY(view.model() != nullptr);
}

void TestLibrary::awaitRows(int expected) {
  QTRY_COMPARE(view.model()->rowCount(view.rootIndex()), expected);
}

QModelIndex TestLibrary::childNamed(const QString &name) const {
  const QModelIndex root = view.rootIndex();
  for (int i = 0; i < view.model()->rowCount(root); i++) {
    const QModelIndex idx = view.model()->index(i, 0, root);
    if (idx.data().toString() == name) {
      return idx;
    }
  }
  return QModelIndex();
}

void TestLibrary::seededLibraryPopulatesTheComboAndTree() {
  QCOMPARE(libswitch.itemData(0).toString(), rock);
  QCOMPARE(libswitch.itemData(1).toString(), jazz);
  QCOMPARE(libswitch.currentIndex(), 0);

  awaitRows(2);
  QVERIFY(childNamed("album").isValid());
  QVERIFY(childNamed("live").isValid());
}

void TestLibrary::radioIsAlwaysTheLastComboEntry() {
  QCOMPARE(libswitch.count(), 3);
  QCOMPARE(libswitch.itemData(2).toString(), DirectoryUi::radioLibraryPath());
}

void TestLibrary::switchingLibraryPersistsTheSelection() {
  libswitch.setCurrentIndex(1);

  QCOMPARE(local->currentLibraryPath(), 1);
  awaitRows(0);

  build();
  QCOMPARE(libswitch.currentIndex(), 1);
}

void TestLibrary::switchingToRadioActivatesTheRadioTree() {
  libswitch.setCurrentIndex(2);

  QCOMPARE(local->currentLibraryPath(), 2);
  QCOMPARE(modus->get(), ModusOperandi::MODUS_LOCALFS);
  QVERIFY(!libsort.isEnabled());
  QTRY_VERIFY(view.model()->rowCount(view.rootIndex()) > 0);
}

void TestLibrary::filteringHidesTopLevelMisses() {
  awaitRows(2);

  search.setText("alb");
  awaitRows(1);
  QVERIFY(childNamed("album").isValid());

  search.clear();
  awaitRows(2);
}

void TestLibrary::selectionCreatesAPlaylistWithTheLibraryRoot() {
  awaitRows(2);
  QSignalSpy spy(library.get(), &DirectoryUi::Controller::createNewPlaylist);
  view.selectionModel()->select(childNamed("album"),
                                QItemSelectionModel::Select | QItemSelectionModel::Rows);

  QVERIFY(library->createPlaylistFromSelection());

  QCOMPARE(spy.count(), 1);
  const auto dirs = spy.first().first().value<QList<QDir>>();
  QCOMPARE(dirs.size(), 1);
  QCOMPARE(dirs.first().absolutePath(), QDir(rock + "/album").absolutePath());
  QCOMPARE(spy.first().at(1).toString(), rock);
}

void TestLibrary::emptySelectionCreatesNothing() {
  QSignalSpy spy(library.get(), &DirectoryUi::Controller::createNewPlaylist);

  QVERIFY(!library->createPlaylistFromSelection());

  QCOMPARE(spy.count(), 0);
}

void TestLibrary::settingsRemoveDropsTheSelectedPath() {
  DirectorySettings dlg({rock, jazz}, *modus, *global, *local);
  auto *list = dlg.findChild<QListView *>(QStringLiteral("listView"));
  auto *remove = dlg.findChild<QPushButton *>(QStringLiteral("pushButtonRemove"));
  QVERIFY(list != nullptr && remove != nullptr);

  list->setCurrentIndex(list->model()->index(0, 0));
  QVERIFY(remove->isEnabled());
  remove->click();

  QCOMPARE(dlg.libraryPaths(), QStringList({jazz}));
}

void TestLibrary::settingsRefusesToRemoveRadio() {
  DirectorySettings dlg({rock, DirectoryUi::radioLibraryPath()}, *modus, *global, *local);
  auto *list = dlg.findChild<QListView *>(QStringLiteral("listView"));
  auto *remove = dlg.findChild<QPushButton *>(QStringLiteral("pushButtonRemove"));

  list->setCurrentIndex(list->model()->index(1, 0));

  QVERIFY(!remove->isEnabled());
  remove->click();
  QCOMPARE(dlg.libraryPaths().size(), 2);
}

void TestLibrary::settingsMoveButtonsReorderAndBound() {
  DirectorySettings dlg({rock, jazz}, *modus, *global, *local);
  auto *list = dlg.findChild<QListView *>(QStringLiteral("listView"));
  auto *up = dlg.findChild<QPushButton *>(QStringLiteral("pushButtonUp"));
  auto *down = dlg.findChild<QPushButton *>(QStringLiteral("pushButtonDown"));

  list->setCurrentIndex(list->model()->index(0, 0));
  QVERIFY(!up->isEnabled());
  QVERIFY(down->isEnabled());

  down->click();
  QCOMPARE(dlg.libraryPaths(), QStringList({jazz, rock}));
  QVERIFY(up->isEnabled());
  QVERIFY(!down->isEnabled());

  up->click();
  QCOMPARE(dlg.libraryPaths(), QStringList({rock, jazz}));
}

MPZ_GUI_TEST_MAIN(TestLibrary)
#include "tst_library.moc"
