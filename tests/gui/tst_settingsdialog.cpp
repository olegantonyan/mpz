#include "fixture.h"

#include "coverart/online/providerchain.h"
#include "lyrics/providerchain.h"
#include "playlist_ui/columnsconfig.h"
#include "settings_ui/settingsdialog.h"
#include "shortcuts.h"
#include "shortcuts_ui/shortcutseditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

// The dialog builds its widgets in code with no object names, so everything is found by type and
// label text. Cache-clear and blacklist buttons open modal dialogs and are never clicked here.
class TestSettingsDialog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void applyPersistsTheCheckboxes();
  void bufferIsShownInKibAndStoredInBytes();
  void rowHeightOverrideStoresZeroWhenOff();
  void columnsRoundTripThroughTheTable();
  void unknownColumnFieldIsKeptNotDropped();
  void unknownLanguageIsAppendedAndSelected();
  void providerListsPutConfiguredFirstAndDropUnknown();
  void uncheckingAProviderRemovesIt();
  void trayToggleIsReportedOnlyOnAChange();
  void waveformToggleIsReportedInBothDirections();
  void applySavesEditedShortcuts();

private:
  GuiTest::ConfigDir config;
  QWidget host;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<Shortcuts> shortcuts;

  static QCheckBox *checkbox(SettingsDialog &dlg, const QString &text);
  static QSpinBox *spinAfter(SettingsDialog &dlg, const QString &checkbox_text);
  static QTableWidget *columnsTable(SettingsDialog &dlg);
  static void apply(SettingsDialog &dlg);
};

void TestSettingsDialog::initTestCase() {
  QVERIFY(config.init());
}

void TestSettingsDialog::init() {
  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  shortcuts = std::make_unique<Shortcuts>(*global, *local, &host);
}

void TestSettingsDialog::cleanup() {
  shortcuts.reset();
  local.reset();
  global.reset();
  QFile::remove(config.path() + "/global.yml");
  QFile::remove(config.path() + "/local.yml");
  QVERIFY(config.init());
}

QCheckBox *TestSettingsDialog::checkbox(SettingsDialog &dlg, const QString &text) {
  for (auto *box : dlg.findChildren<QCheckBox *>()) {
    if (box->text() == text) {
      return box;
    }
  }
  return nullptr;
}

QSpinBox *TestSettingsDialog::spinAfter(SettingsDialog &dlg, const QString &checkbox_text) {
  auto *box = checkbox(dlg, checkbox_text);
  return box ? box->parentWidget()->findChild<QSpinBox *>() : nullptr;
}

QTableWidget *TestSettingsDialog::columnsTable(SettingsDialog &dlg) {
  auto *editor = dlg.findChild<ShortcutsEditor *>();
  for (auto *table : dlg.findChildren<QTableWidget *>()) {
    if (!editor->isAncestorOf(table)) {
      return table;
    }
  }
  return nullptr;
}

void TestSettingsDialog::apply(SettingsDialog &dlg) {
  dlg.findChild<QDialogButtonBox *>(QString(), Qt::FindDirectChildrenOnly)->button(QDialogButtonBox::Apply)->click();
}

void TestSettingsDialog::applyPersistsTheCheckboxes() {
  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *stop = checkbox(dlg, "Stop playback when current track or playlist is removed");
  auto *headers = checkbox(dlg, "Show column headers");
  auto *single = checkbox(dlg, "Single instance mode");
  QVERIFY(stop != nullptr && headers != nullptr && single != nullptr);
  QCOMPARE(stop->isChecked(), false);

  stop->setChecked(true);
  headers->setChecked(!headers->isChecked());
  single->setChecked(true);
  const bool expected_headers = headers->isChecked();
  apply(dlg);

  QCOMPARE(global->stopWhenTrackRemoved(), true);
  QCOMPARE(global->showPlaylistHeaders(), expected_headers);
  QCOMPARE(global->singleInstance(), true);
}

void TestSettingsDialog::bufferIsShownInKibAndStoredInBytes() {
  global->saveStreamBufferSize(256 * 1024);
  SettingsDialog dlg(*global, *local, shortcuts.get());

  QSpinBox *buffer = nullptr;
  for (auto *spin : dlg.findChildren<QSpinBox *>()) {
    if (spin->value() == 256) {
      buffer = spin;
    }
  }
  QVERIFY(buffer != nullptr);

  buffer->setValue(512);
  apply(dlg);

  QCOMPARE(global->streamBufferSize(), 512 * 1024);
}

void TestSettingsDialog::rowHeightOverrideStoresZeroWhenOff() {
  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *box = checkbox(dlg, "Override theme's playlist row height:");
  auto *spin = spinAfter(dlg, "Override theme's playlist row height:");
  QVERIFY(box != nullptr && spin != nullptr);
  QVERIFY(!box->isChecked());
  QVERIFY(!spin->isEnabled());

  box->setChecked(true);
  QVERIFY(spin->isEnabled());
  spin->setValue(30);
  apply(dlg);
  QCOMPARE(global->playlistRowHeight(), 30);

  box->setChecked(false);
  apply(dlg);
  QCOMPARE(global->playlistRowHeight(), 0);
}

void TestSettingsDialog::columnsRoundTripThroughTheTable() {
  // The table mirrors what is stored, and nothing is stored on a fresh profile.
  {
    SettingsDialog empty(*global, *local, shortcuts.get());
    QCOMPARE(columnsTable(empty)->rowCount(), 0);
  }
  QVERIFY(global->saveColumnsConfig(PlaylistUi::ColumnsConfig()));

  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *table = columnsTable(dlg);
  QVERIFY(table != nullptr);
  QVERIFY(table->rowCount() > 0);

  qobject_cast<QSpinBox *>(table->cellWidget(0, 1))->setValue(40);
  qobject_cast<QComboBox *>(table->cellWidget(0, 2))->setCurrentIndex(1); // right
  table->item(0, 3)->setCheckState(Qt::Checked);
  const QString field = qobject_cast<QComboBox *>(table->cellWidget(0, 0))->currentText();
  apply(dlg);

  const auto cfg = global->columnsConfig();
  QCOMPARE(cfg.field(1), field);
  QCOMPARE(cfg.width(1), 0.4);
  QCOMPARE(cfg.align(1), Qt::Alignment(Qt::AlignVCenter | Qt::AlignRight));
  QVERIFY(cfg.stretch(1));
}

void TestSettingsDialog::unknownColumnFieldIsKeptNotDropped() {
  auto cfg = global->columnsConfig();
  QVector<QString> fields = {"artist", "from_the_future"};
  cfg.setFields(fields);
  cfg.setWidths({0.5, 0.5});
  cfg.setAligns({Qt::AlignVCenter, Qt::AlignVCenter});
  cfg.setStretches({false, true});
  QVERIFY(global->saveColumnsConfig(cfg));

  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *table = columnsTable(dlg);

  QCOMPARE(table->rowCount(), 2);
  QCOMPARE(qobject_cast<QComboBox *>(table->cellWidget(1, 0))->currentText(), QString("from_the_future"));

  apply(dlg);
  QCOMPARE(global->columnsConfig().field(2), QString("from_the_future"));
}

void TestSettingsDialog::unknownLanguageIsAppendedAndSelected() {
  global->saveLanguage("kl");

  SettingsDialog dlg(*global, *local, shortcuts.get());
  QComboBox *language = nullptr;
  for (auto *combo : dlg.findChildren<QComboBox *>()) {
    if (combo->currentData().toString() == "kl") {
      language = combo;
    }
  }

  QVERIFY(language != nullptr);
  QCOMPARE(language->currentIndex(), language->count() - 1);
  apply(dlg);
  QCOMPARE(global->language(), QString("kl"));
}

void TestSettingsDialog::providerListsPutConfiguredFirstAndDropUnknown() {
  const QStringList known = Lyrics::ProviderChain::knownProviders();
  QVERIFY(known.size() >= 2);
  QVERIFY(global->saveLyricsProviders({known.last(), "embedded"}));

  SettingsDialog dlg(*global, *local, shortcuts.get());
  QListWidget *lyrics = nullptr;
  for (auto *list : dlg.findChildren<QListWidget *>()) {
    if (list->count() == known.size()) {
      lyrics = list;
      break;
    }
  }
  QVERIFY(lyrics != nullptr);

  QCOMPARE(lyrics->item(0)->data(Qt::UserRole).toString(), known.last());
  QCOMPARE(lyrics->item(0)->checkState(), Qt::Checked);
  QCOMPARE(lyrics->item(1)->checkState(), Qt::Unchecked);

  apply(dlg);
  // "embedded" is not a chain provider any more, so the save drops it.
  QCOMPARE(global->lyricsProviders(), QStringList({known.last()}));
}

void TestSettingsDialog::uncheckingAProviderRemovesIt() {
  const QStringList known = CoverArt::Online::ProviderChain::knownProviders();
  QVERIFY(global->saveCoverProviders(known));

  SettingsDialog dlg(*global, *local, shortcuts.get());
  QListWidget *covers = nullptr;
  for (auto *list : dlg.findChildren<QListWidget *>()) {
    if (list->count() == known.size() && list->item(0)->data(Qt::UserRole).toString() == known.first()) {
      covers = list;
      break;
    }
  }
  QVERIFY(covers != nullptr);

  covers->item(0)->setCheckState(Qt::Unchecked);
  apply(dlg);

  QVERIFY(!global->coverProviders().contains(known.first()));
  QCOMPARE(global->coverProviders().size(), known.size() - 1);
}

void TestSettingsDialog::trayToggleIsReportedOnlyOnAChange() {
  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *tray = dlg.findChildren<QCheckBox *>().first();
  for (auto *box : dlg.findChildren<QCheckBox *>()) {
    if (box->text().contains("tray icon") || box->text().contains("menu bar")) {
      tray = box;
      break;
    }
  }
  QSignalSpy spy(&dlg, &SettingsDialog::trayIconToggled);

  apply(dlg);
  QCOMPARE(spy.count(), 0);

  tray->setChecked(!tray->isChecked());
  apply(dlg);
  QCOMPARE(spy.count(), 1);

  apply(dlg);
  QCOMPARE(spy.count(), 1);
}

void TestSettingsDialog::waveformToggleIsReportedInBothDirections() {
  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *waveform = checkbox(dlg, "Show waveform in the seekbar");
  if (waveform == nullptr) {
    QSKIP("gapless disabled, no waveform setting");
  }
  QSignalSpy spy(&dlg, &SettingsDialog::waveformToggled);
  QVERIFY(waveform->isChecked());

  waveform->setChecked(false);
  apply(dlg);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.last().first().toBool(), false);
  QVERIFY(global->waveformDisabled());

  waveform->setChecked(true);
  apply(dlg);
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.last().first().toBool(), true);
  QVERIFY(!global->waveformDisabled());
}

void TestSettingsDialog::applySavesEditedShortcuts() {
  const auto &spec = Shortcuts::defaults().first();
  QVERIFY(!spec.description.isEmpty());

  SettingsDialog dlg(*global, *local, shortcuts.get());
  auto *table = dlg.findChild<ShortcutsEditor *>()->findChild<QTableWidget *>();
  QVERIFY(table != nullptr);
  QCOMPARE(table->item(0, 0)->text(), spec.description);

  qobject_cast<QKeySequenceEdit *>(table->cellWidget(0, 1))
      ->setKeySequence(QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));
  apply(dlg);

  QCOMPARE(global->shortcuts().value(spec.key), QString("Ctrl+Alt+Z"));
  QCOMPARE(shortcuts->sequenceFor(spec.action), QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));
}

MPZ_GUI_TEST_MAIN(TestSettingsDialog)
#include "tst_settingsdialog.moc"
