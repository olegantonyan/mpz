#include "fixture.h"

#include "config/global.h"
#include "config/local.h"
#include "ipc/instance.h"
#include "mainwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTableView>
#include <QToolButton>
#include <QTreeView>

namespace {
  QPoint checkboxIndicator(QCheckBox *box) {
    QStyleOptionButton opt;
    opt.initFrom(box);
    return box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, box).center();
  }
}

// One window for the whole binary: Config::Global, Config::Local and
// ModusOperandi are single-instance guarded, and CoverArt::Covers is a
// process-wide singleton that captures the first window's ModusOperandi.
// The window is never close()d either -- closeEvent ends in qApp->quit().
class TestMainWindow : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void buildsWithControlsAndViews();
  void followCursorClickWritesConfig();
  void orderComboBoxKeyboardWritesConfig();

private:
  GuiTest::ConfigDir config;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<IPC::Instance> instance;
  std::unique_ptr<MainWindow> window;
};

void TestMainWindow::initTestCase() {
  QVERIFY(config.init({config.path() + "/library"}));
  QVERIFY(QDir().mkpath(config.path() + "/library"));

  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  instance = std::make_unique<IPC::Instance>();
  window = std::make_unique<MainWindow>(QStringList(), instance.get(), *local, *global);
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window.get()));
}

void TestMainWindow::cleanupTestCase() {
  window.reset();
  instance.reset();
  local.reset();
  global.reset();
}

void TestMainWindow::buildsWithControlsAndViews() {
  for (const char *name : {"playButton", "pauseButton", "stopButton", "prevButton", "nextButton"}) {
    auto *button = window->findChild<QToolButton *>(QString::fromLatin1(name));
    QVERIFY2(button != nullptr, name);
    QVERIFY2(button->isVisible(), name);
  }

  QVERIFY(window->findChild<QTreeView *>(QStringLiteral("treeView")) != nullptr);
  QVERIFY(window->findChild<QListView *>(QStringLiteral("listView")) != nullptr);
  QVERIFY(window->findChild<QTableView *>(QStringLiteral("tableView")) != nullptr);
  QVERIFY(window->findChild<QComboBox *>(QStringLiteral("comboBoxLibraries")) != nullptr);
  QVERIFY(window->findChild<QLineEdit *>(QStringLiteral("treeViewSearch")) != nullptr);
  QVERIFY(window->statusBar() != nullptr);
}

void TestMainWindow::followCursorClickWritesConfig() {
  auto *box = window->findChild<QCheckBox *>(QStringLiteral("followCursorCheckBox"));
  QVERIFY(box != nullptr);
  const bool expected = !box->isChecked();

  QTest::mouseClick(box, Qt::LeftButton, Qt::NoModifier, checkboxIndicator(box));

  QCOMPARE(box->isChecked(), expected);
  QCOMPARE(global->playbackFollowCursor(), expected);
}

void TestMainWindow::orderComboBoxKeyboardWritesConfig() {
  auto *combo = window->findChild<QComboBox *>(QStringLiteral("orderComboBox"));
  QVERIFY(combo != nullptr);
  combo->setCurrentIndex(0);

  QTest::keyClick(combo, Qt::Key_Down);

  QCOMPARE(combo->currentIndex(), 1);
  QCOMPARE(global->playbackOrder(), QString("random"));
}

MPZ_GUI_TEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
