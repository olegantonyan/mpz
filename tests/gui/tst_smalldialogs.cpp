#include "fixture.h"

#include "backgroundtasks.h"
#include "backgroundtasksbutton.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "directory_ui/addmpddialog.h"
#endif
#include "directory_ui/radiostationsdialog.h"
#include "icons.h"
#include "modusoperandi.h"
#include "playback_log_ui/playbacklogdialog.h"
#include "playback_log_ui/playbackloguicontroller.h"
#include "slidingbanner.h"
#include "sort_ui/sortingpresetsdialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QTableWidget>

namespace {
  Radio::Station station(const QString &id, const QString &name) {
    Radio::Station st;
    st.id = id;
    st.name = name;
    st.url = "http://radio.example/" + id;
    return st;
  }
}

// The small dialogs and widgets, grouped so they share one app instance.
class TestSmallDialogs : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();
  void sortingPresetsFormatsTheList();
  void sortingPresetsAddAndRemoveAreGuarded();
  void sortingPresetsTestEmitsTheCriteria();
  void playbackLogDropsEmptyStreamsAndDuplicates();
  void playbackLogSizeFallsBackTo100();
  void playbackLogDialogLabelsAndJump();
  void radioStationsTableMirrorsTheVector();
#ifdef ENABLE_MPD_SUPPORT
  void addMpdDialogRoundTripsTheUrl();
#endif
  void backgroundTasksButtonFollowsTheTaskList();
  void everyIconResolves();

private:
  GuiTest::ConfigDir config;
  SlidingBanner banner;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;

  void openConfig();
};

void TestSmallDialogs::initTestCase() {
  QVERIFY(config.init());
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
}

void TestSmallDialogs::cleanup() {
  global.reset();
  QFile::remove(config.path() + "/global.yml");
}

void TestSmallDialogs::openConfig() {
  global = std::make_unique<Config::Global>();
}

void TestSmallDialogs::sortingPresetsFormatsTheList() {
  SortingPresetsDialog dlg({SortingPreset("", "Artist"),
                            SortingPreset("Album view", "Album / Title"),
                            SortingPreset("Title", "Title")});
  auto *list = dlg.findChild<QListView *>(QStringLiteral("listViewPresets"));

  QCOMPARE(list->model()->rowCount(), 3);
  QCOMPARE(list->model()->index(0, 0).data().toString(), QString("Artist"));
  QCOMPARE(list->model()->index(1, 0).data().toString(), QString("Album / Title (Album view)"));
  // A name equal to the criteria is not repeated.
  QCOMPARE(list->model()->index(2, 0).data().toString(), QString("Title"));
}

void TestSmallDialogs::sortingPresetsAddAndRemoveAreGuarded() {
  SortingPresetsDialog dlg({SortingPreset("", "Artist")});
  auto *list = dlg.findChild<QListView *>(QStringLiteral("listViewPresets"));
  auto *line = dlg.findChild<QLineEdit *>(QStringLiteral("lineEditNewPreset"));

  dlg.findChild<QPushButton *>(QStringLiteral("buttonAdd"))->click();
  QCOMPARE(dlg.currentPresets().size(), 1);

  line->setText("Year / Album");
  dlg.findChild<QPushButton *>(QStringLiteral("buttonAdd"))->click();
  QCOMPARE(dlg.currentPresets().size(), 2);
  QCOMPARE(list->model()->rowCount(), 2);

  // Nothing selected: remove is a no-op.
  dlg.findChild<QPushButton *>(QStringLiteral("buttonRemove"))->click();
  QCOMPARE(dlg.currentPresets().size(), 2);

  list->setCurrentIndex(list->model()->index(0, 0));
  dlg.findChild<QPushButton *>(QStringLiteral("buttonRemove"))->click();
  QCOMPARE(dlg.currentPresets().size(), 1);
  QCOMPARE(dlg.currentPresets().first().second, QString("Year / Album"));
}

void TestSmallDialogs::sortingPresetsTestEmitsTheCriteria() {
  SortingPresetsDialog dlg({});
  QSignalSpy spy(&dlg, &SortingPresetsDialog::triggeredSort);
  auto *test = dlg.findChild<QPushButton *>(QStringLiteral("buttonTest"));

  test->click();
  QCOMPARE(spy.count(), 0);

  dlg.findChild<QLineEdit *>(QStringLiteral("lineEditNewPreset"))->setText("-Year");
  test->click();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toString(), QString("-Year"));
}

void TestSmallDialogs::playbackLogDropsEmptyStreamsAndDuplicates() {
  openConfig();
  PlaybackLogUi::Controller controller(*local, *global);
  auto *model = controller.findChild<PlaybackLogUi::Model *>();
  QVERIFY(model != nullptr);

  controller.append(Track(QUrl("http://radio.example/live"), "radio://x"));
  QCOMPARE(model->rowCount(), 0);

  const Track track = GuiTest::track("song");
  controller.append(track);
  controller.append(track);
  QCOMPARE(model->rowCount(), 1);
  QCOMPARE(model->last().text, track.formattedTitle());

  controller.append(GuiTest::track("other"));
  QCOMPARE(model->rowCount(), 2);
}

void TestSmallDialogs::playbackLogSizeFallsBackTo100() {
  openConfig();
  QCOMPARE(global->playbackLogSize(), 0);
  PlaybackLogUi::Controller controller(*local, *global);
  auto *model = controller.findChild<PlaybackLogUi::Model *>();

  for (int i = 0; i < 101; i++) {
    controller.append(GuiTest::track(QString("song%1").arg(i)));
  }
  QCOMPARE(model->rowCount(), 100);
}

void TestSmallDialogs::playbackLogDialogLabelsAndJump() {
  openConfig();
  PlaybackLogUi::Model model(*local, 10);
  const Track track = GuiTest::track("song");
  model.append(PlaybackLogUi::Item(track.uid(), track.formattedTitle()));

  PlaybackLogDialog dlg(&model);
  QSignalSpy jumped(&dlg, &PlaybackLogDialog::jumpToTrack);

  QVERIFY(dlg.findChild<QLabel *>(QStringLiteral("labelTotalPlayTime"))
              ->text().startsWith("Total time played: "));
  QVERIFY(dlg.findChild<QLabel *>(QStringLiteral("labelThisSessionPlayTime"))
              ->text().startsWith("This session time played: 00:00"));

  auto *view = dlg.findChild<QTableView *>(QStringLiteral("tableView"));
  emit view->doubleClicked(view->model()->index(0, 0));

  QCOMPARE(jumped.count(), 1);
  QCOMPARE(jumped.first().first().toULongLong(), track.uid());
}

void TestSmallDialogs::radioStationsTableMirrorsTheVector() {
  DirectoryUi::RadioStationsDialog dlg({station("a", "Alpha"), station("b", "Beta")});
  auto *table = dlg.findChild<QTableWidget *>();
  QVERIFY(table != nullptr);
  QCOMPARE(table->rowCount(), 2);
  QCOMPARE(dlg.stations().size(), 2);

  auto *remove = dlg.findChild<QPushButton *>(QStringLiteral("buttonRemoveStation"));
  if (remove == nullptr) {
    for (auto *button : dlg.findChildren<QPushButton *>()) {
      if (button->text().contains("Remove")) {
        remove = button;
      }
    }
  }
  QVERIFY(remove != nullptr);

  // Nothing selected: bounds-guarded no-op.
  remove->click();
  QCOMPARE(dlg.stations().size(), 2);

  table->setCurrentCell(0, 0);
  remove->click();
  QCOMPARE(dlg.stations().size(), 1);
  QCOMPARE(dlg.stations().first().id, QString("b"));
  QCOMPARE(table->rowCount(), 1);
}

#ifdef ENABLE_MPD_SUPPORT
void TestSmallDialogs::addMpdDialogRoundTripsTheUrl() {
  DirectoryUi::AddMpdDialog dlg(modus->mpd_client);

  dlg.setUrl("mpd://host:6600");
  QCOMPARE(dlg.url(), QString("mpd://host:6600"));

  // A bare host gets the scheme prepended.
  auto *host = dlg.findChildren<QLineEdit *>().first();
  host->setText("otherhost:6601");
  QCOMPARE(dlg.url(), QString("mpd://otherhost:6601"));

  dlg.setUrl("mpd://user:secret@host:6600");
  QVERIFY(dlg.url().contains("secret"));
  QCOMPARE(QUrl(dlg.url()).host(), QString("host"));
}
#endif

void TestSmallDialogs::backgroundTasksButtonFollowsTheTaskList() {
  BackgroundTasks tasks;
  BackgroundTasksButton button;
  button.setTasks(&tasks);

  QVERIFY(button.isHidden());

  const quint64 one = tasks.begin("scanning");
  QTRY_VERIFY(!button.isHidden());
  QCOMPARE(button.toolTip(), QString("scanning"));

  tasks.begin("downloading");
  QTRY_COMPARE(button.toolTip(), QString("2 background operations"));

  tasks.end(one);
  QTRY_COMPARE(button.toolTip(), QString("downloading"));

  tasks.end(tasks.tasks().first().id);
  QTRY_VERIFY(button.isHidden());
}

void TestSmallDialogs::everyIconResolves() {
  const QVector<Icons::Icon> all = {
    Icons::Icon::Play, Icons::Icon::Pause, Icons::Icon::Stop, Icons::Icon::Next, Icons::Icon::Prev,
    Icons::Icon::VolumeMuted, Icons::Icon::VolumeLow, Icons::Icon::VolumeMedium,
    Icons::Icon::VolumeHigh, Icons::Icon::Headphones, Icons::Icon::Settings, Icons::Icon::Menu,
    Icons::Icon::Sort, Icons::Icon::Trash, Icons::Icon::Folder, Icons::Icon::FolderReveal,
    Icons::Icon::Reload, Icons::Icon::Save, Icons::Icon::Info, Icons::Icon::Details,
    Icons::Icon::Help, Icons::Icon::Cancel, Icons::Icon::Copy, Icons::Icon::Edit,
    Icons::Icon::OpenFile, Icons::Icon::NewPlaylist, Icons::Icon::AddToPlaylist,
    Icons::Icon::MoveToPlaylist, Icons::Icon::Spinner, Icons::Icon::Radio,
    Icons::Icon::DynamicRange
  };

  for (const auto icon : all) {
    const int value = static_cast<int>(icon);
    QVERIFY2(!Icons::get(icon).isNull(), qPrintable(QString::number(value)));
    QVERIFY2(!Icons::pixmap(icon, QSize(16, 16)).isNull(), qPrintable(QString::number(value)));
  }
}

MPZ_GUI_TEST_MAIN(TestSmallDialogs)
#include "tst_smalldialogs.moc"
