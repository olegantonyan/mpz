#include "fixture.h"

#include "config/global.h"
#include "config/local.h"
#include "ipc/instance.h"
#include "mainwindow.h"

#include <QFileOpenEvent>
#include <QListView>
#include <QTableView>
#include <QToolButton>

// Opening files from the command line: MainWindow preloads its argument list in
// the constructor, so the window is built once here with a directory of tracks.
class TestFileOpen : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void commandLineArgumentBecomesAPlaylist();
  void filesFromARunningInstanceAppendAnotherPlaylist();
  void fileOpenEventQueuesUntilDrained();
  void fileOpenEventIsDeliveredWhenConnected();

private:
  GuiTest::ConfigDir config;
  QString first_dir, second_dir;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<IPC::Instance> instance;
  std::unique_ptr<MainWindow> window;

  QListView *playlists() const;
  QStringList playlistNames() const;
  // Files named on the command line have no library root to strip, so the
  // playlist is named after the absolute path, clamped by Playlist::rename.
  static QString nameFor(const QString &dir);
};

void TestFileOpen::initTestCase() {
  QVERIFY(config.init({config.path() + "/library"}));
  QVERIFY(QDir().mkpath(config.path() + "/library"));
  first_dir = config.path() + "/library/from_cli";
  second_dir = config.path() + "/library/from_ipc";
  QVERIFY(GuiTest::copyAudioFixtures(first_dir));
  QVERIFY(GuiTest::copyAudioFixtures(second_dir));

  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  instance = std::make_unique<IPC::Instance>();
  window = std::make_unique<MainWindow>(QStringList{first_dir}, instance.get(), *local, *global);
  window->show();
  QVERIFY(QTest::qWaitForWindowExposed(window.get()));

  QTRY_COMPARE(playlistNames(), QStringList({nameFor(first_dir)}));

  // The preloaded playlist starts playing; stop it so nothing holds the device.
  window->findChild<QToolButton *>(QStringLiteral("stopButton"))->click();
}

void TestFileOpen::cleanupTestCase() {
  window.reset();
  instance.reset();
  local.reset();
  global.reset();
}

QListView *TestFileOpen::playlists() const {
  return window->findChild<QListView *>(QStringLiteral("listView"));
}

QString TestFileOpen::nameFor(const QString &dir) {
  Playlist::Playlist clamp;
  return clamp.rename(QDir(dir).absolutePath().mid(1));
}

QStringList TestFileOpen::playlistNames() const {
  QStringList result;
  auto *model = playlists()->model();
  if (model == nullptr) {
    return result;
  }
  for (int i = 0; i < model->rowCount(); i++) {
    result << model->index(i, 0).data().toString();
  }
  return result;
}

void TestFileOpen::commandLineArgumentBecomesAPlaylist() {
  QCOMPARE(playlistNames(), QStringList({nameFor(first_dir)}));
  QTRY_COMPARE(window->findChild<QTableView *>(QStringLiteral("tableView"))->model()->rowCount(), 3);
}

void TestFileOpen::filesFromARunningInstanceAppendAnotherPlaylist() {
  emit instance->load_files_received({second_dir});

  QTRY_COMPARE(playlistNames(), QStringList({nameFor(first_dir), nameFor(second_dir)}));
  window->findChild<QToolButton *>(QStringLiteral("stopButton"))->click();
}

void TestFileOpen::fileOpenEventQueuesUntilDrained() {
  auto *app = static_cast<MpzApplication *>(qApp);
  QFileOpenEvent event(QStringLiteral("/music/dropped.mp3"));

  QCoreApplication::sendEvent(app, &event);

  QCOMPARE(app->drainPendingFiles(), QStringList({"/music/dropped.mp3"}));
  QVERIFY(app->drainPendingFiles().isEmpty());
}

void TestFileOpen::fileOpenEventIsDeliveredWhenConnected() {
  auto *app = static_cast<MpzApplication *>(qApp);
  QSignalSpy spy(app, &MpzApplication::filesOpened);
  QFileOpenEvent event(QStringLiteral("/music/opened.mp3"));

  QCoreApplication::sendEvent(app, &event);

  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toStringList(), QStringList({"/music/opened.mp3"}));
  QVERIFY(app->drainPendingFiles().isEmpty());
}

MPZ_GUI_TEST_MAIN(TestFileOpen)
#include "tst_fileopen.moc"
