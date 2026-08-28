#include "mainwindow.h"
#include "config/global.h"
#include "config/local.h"
#include "ipc/instance.h"

#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QStyle>
#include <QStandardPaths>
#include <QStyleOptionButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTreeView>

namespace {
  // Config::Global and Config::Local are single-instance guarded, so every test
  // gets its own scope and tears the window down before the next one builds.
  struct App {
    Config::Global global;
    Config::Local local;
    IPC::Instance instance;
    MainWindow window;

    App() : window(QStringList(), &instance, local, global) {}

    void show() {
      window.show();
      QVERIFY(QTest::qWaitForWindowExposed(&window));
    }
  };

  QPoint checkboxIndicator(QCheckBox *box) {
    QStyleOptionButton opt;
    opt.initFrom(box);
    return box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, box).center();
  }
}

class TestMainWindow : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void buildsWithControlsAndViews();
  void followCursorClickPersists();
  void orderComboBoxKeyboardPersists();

private:
  QTemporaryDir config_dir;
};

void TestMainWindow::initTestCase() {
  QVERIFY(config_dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", config_dir.path().toUtf8());
  QStandardPaths::setTestModeEnabled(true);
}

void TestMainWindow::buildsWithControlsAndViews() {
  App app;
  app.show();

  for (const char *name : {"playButton", "pauseButton", "stopButton", "prevButton", "nextButton"}) {
    auto *button = app.window.findChild<QToolButton *>(QString::fromLatin1(name));
    QVERIFY2(button != nullptr, name);
    QVERIFY2(button->isVisible(), name);
  }

  QVERIFY(app.window.findChild<QTreeView *>(QStringLiteral("treeView")) != nullptr);
  QVERIFY(app.window.findChild<QTableView *>(QStringLiteral("tableView")) != nullptr);
  QVERIFY(app.window.statusBar() != nullptr);
}

void TestMainWindow::followCursorClickPersists() {
  bool expected = false;

  {
    App app;
    app.show();

    auto *box = app.window.findChild<QCheckBox *>(QStringLiteral("followCursorCheckBox"));
    QVERIFY(box != nullptr);
    const bool before = box->isChecked();

    QTest::mouseClick(box, Qt::LeftButton, Qt::NoModifier, checkboxIndicator(box));

    expected = !before;
    QCOMPARE(box->isChecked(), expected);
    QCOMPARE(app.global.playbackFollowCursor(), expected);
  }

  App reopened;
  auto *box = reopened.window.findChild<QCheckBox *>(QStringLiteral("followCursorCheckBox"));
  QVERIFY(box != nullptr);
  QCOMPARE(box->isChecked(), expected);
}

void TestMainWindow::orderComboBoxKeyboardPersists() {
  App app;
  app.show();

  auto *combo = app.window.findChild<QComboBox *>(QStringLiteral("orderComboBox"));
  QVERIFY(combo != nullptr);
  combo->setCurrentIndex(0);

  QTest::keyClick(combo, Qt::Key_Down);

  QCOMPARE(combo->currentIndex(), 1);
  QCOMPARE(app.global.playbackOrder(), QString("random"));
}

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
