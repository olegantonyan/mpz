#include "fixture.h"
#include "server.h"

#include "config/global.h"
#include "config/local.h"
#include "ipc/instance.h"
#include "mainwindow.h"

#include <QComboBox>
#include <QListView>
#include <QTreeView>
#include <QThreadPool>
#include <QtTest>

// The whole app pointed at a real mpd. One window for the binary: the config objects and ModusOperandi are single-instance
// guarded, CoverArt::Covers is a process-wide singleton, and closeEvent ends in qApp->quit() so the window is never closed here.
class TestMpdMainWindow : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();

  void theLibraryComboShowsTheMpdUrl();
  void libraryTreeShowsTheMpdRoot();
  void thePlaylistsViewStartsFromTheServer();
  void aPlaylistCreatedOnTheServerShowsUpInTheList();

private:
  QTreeView *treeView() const;
  QStringList treeTopLevel() const;

  MpdTest::Server server;
  GuiTest::ConfigDir config;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<IPC::Instance> instance;
  std::unique_ptr<MainWindow> window;
};

void TestMpdMainWindow::initTestCase() {
  if (!MpdTest::Server::installed()) {
    QSKIP("mpd is not installed");
  }
  MpdTest::registerMetaTypes();
  QVERIFY2(server.start(), qPrintable(server.failReason()));

  // The directory controller opens the connection while the window is built.
  QVERIFY(config.init({server.url().toString()}));

  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  instance = std::make_unique<IPC::Instance>();
  window = std::make_unique<MainWindow>(QStringList(), instance.get(), *local, *global);
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window.get()));
}

void TestMpdMainWindow::cleanupTestCase() {
  QThreadPool::globalInstance()->waitForDone();
  window.reset();
  instance.reset();
  local.reset();
  global.reset();
}

QTreeView *TestMpdMainWindow::treeView() const {
  return window->findChild<QTreeView *>(QStringLiteral("treeView"));
}

QStringList TestMpdMainWindow::treeTopLevel() const {
  QStringList names;
  auto *view = treeView();
  if (!view || !view->model()) {
    return names;
  }
  const QModelIndex root = view->rootIndex();
  for (int i = 0; i < view->model()->rowCount(root); i++) {
    names << view->model()->index(i, 0, root).data().toString();
  }
  return names;
}

void TestMpdMainWindow::theLibraryComboShowsTheMpdUrl() {
  auto *combo = window->findChild<QComboBox *>(QStringLiteral("comboBoxLibraries"));
  QVERIFY(combo != nullptr);

  QCOMPARE(combo->currentText(), server.url().toString());
}

void TestMpdMainWindow::libraryTreeShowsTheMpdRoot() {
  QVERIFY(treeView() != nullptr);

  QTRY_COMPARE_WITH_TIMEOUT(treeTopLevel().size(), 4, 30000);
  for (const QString &expected : {"wav", "tagged", "covered", "embedded"}) {
    QVERIFY2(treeTopLevel().contains(expected), qPrintable(treeTopLevel().join(", ")));
  }
}

void TestMpdMainWindow::thePlaylistsViewStartsFromTheServer() {
  auto *list = window->findChild<QListView *>(QStringLiteral("listView"));
  QVERIFY(list != nullptr);
  QVERIFY(list->model() != nullptr);

  QTRY_COMPARE_WITH_TIMEOUT(list->model()->rowCount(QModelIndex()), 0, 30000);
}

void TestMpdMainWindow::aPlaylistCreatedOnTheServerShowsUpInTheList() {
  auto *list = window->findChild<QListView *>(QStringLiteral("listView"));
  QVERIFY(list != nullptr);

  server.command("save \"made_elsewhere\"");

  QTRY_VERIFY_WITH_TIMEOUT(list->model()->rowCount(QModelIndex()) == 1, 30000);
  QCOMPARE(list->model()->index(0, 0, QModelIndex()).data().toString(), QString("made_elsewhere"));
}

MPZ_GUI_TEST_MAIN(TestMpdMainWindow)

#include "tst_mpdmainwindow.moc"
