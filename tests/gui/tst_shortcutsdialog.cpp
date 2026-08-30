#include "fixture.h"

#include "shortcuts.h"
#include "shortcuts_ui/shortcutsdialog.h"

#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

class TestShortcutsDialog : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void listsOnlyDescribedShortcuts();
  void multiChordEntryIsTruncatedToTheFirstCombination();
  void takingABoundKeyClearsTheOtherRow();
  void restoreDefaultsDoesNotTripTheStealLogic();
  void applyPersistsOnlyChangedKeys();

private:
  GuiTest::ConfigDir config;
  QWidget host;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<Shortcuts> shortcuts;

  int rowFor(ShortcutsDialog &dlg, const QString &description) const;
  static QKeySequenceEdit *editorAt(ShortcutsDialog &dlg, int row);
};

void TestShortcutsDialog::initTestCase() {
  QVERIFY(config.init());
}

void TestShortcutsDialog::init() {
  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  shortcuts = std::make_unique<Shortcuts>(*global, *local, &host);
}

void TestShortcutsDialog::cleanup() {
  shortcuts.reset();
  local.reset();
  global.reset();
  QFile::remove(config.path() + "/global.yml");
}

int TestShortcutsDialog::rowFor(ShortcutsDialog &dlg, const QString &description) const {
  auto *table = dlg.findChild<QTableWidget *>(QStringLiteral("tableWidget"));
  for (int i = 0; i < table->rowCount(); i++) {
    if (table->item(i, 0)->text() == description) {
      return i;
    }
  }
  return -1;
}

QKeySequenceEdit *TestShortcutsDialog::editorAt(ShortcutsDialog &dlg, int row) {
  auto *table = dlg.findChild<QTableWidget *>(QStringLiteral("tableWidget"));
  return qobject_cast<QKeySequenceEdit *>(table->cellWidget(row, 1));
}

void TestShortcutsDialog::listsOnlyDescribedShortcuts() {
  ShortcutsDialog dlg(shortcuts.get());
  auto *table = dlg.findChild<QTableWidget *>(QStringLiteral("tableWidget"));
  QVERIFY(table != nullptr);

  int described = 0;
  for (const auto &spec : shortcuts->specs()) {
    if (!spec.description.isEmpty()) {
      described++;
    }
  }
  QCOMPARE(table->rowCount(), described);
  QVERIFY(described > 0);
  QVERIFY(editorAt(dlg, 0) != nullptr);
}

void TestShortcutsDialog::multiChordEntryIsTruncatedToTheFirstCombination() {
  ShortcutsDialog dlg(shortcuts.get());
  auto *editor = editorAt(dlg, 0);

  editor->setKeySequence(QKeySequence::fromString("Ctrl+K, Ctrl+L", QKeySequence::PortableText));

  QCOMPARE(editor->keySequence().count(), 1);
  QCOMPARE(editor->keySequence(), QKeySequence::fromString("Ctrl+K", QKeySequence::PortableText));
}

void TestShortcutsDialog::takingABoundKeyClearsTheOtherRow() {
  ShortcutsDialog dlg(shortcuts.get());
  auto *warning = dlg.findChild<QLabel *>(QStringLiteral("warningLabel"));
  auto *first = editorAt(dlg, 0);
  auto *second = editorAt(dlg, 1);
  const QKeySequence taken = first->keySequence();
  QVERIFY(!taken.isEmpty());

  second->setKeySequence(taken);

  QVERIFY(first->keySequence().isEmpty());
  QCOMPARE(second->keySequence(), taken);
  QVERIFY(warning->text().contains(taken.toString(QKeySequence::NativeText)));
}

void TestShortcutsDialog::restoreDefaultsDoesNotTripTheStealLogic() {
  ShortcutsDialog dlg(shortcuts.get());
  auto *warning = dlg.findChild<QLabel *>(QStringLiteral("warningLabel"));
  auto *buttons = dlg.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
  editorAt(dlg, 0)->clear();
  editorAt(dlg, 1)->setKeySequence(QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));

  buttons->button(QDialogButtonBox::RestoreDefaults)->click();

  QVERIFY(warning->text().isEmpty());
  for (const auto &spec : Shortcuts::defaults()) {
    if (spec.description.isEmpty()) {
      continue;
    }
    const int row = rowFor(dlg, spec.description);
    QVERIFY2(row >= 0, qPrintable(spec.description));
    QCOMPARE(editorAt(dlg, row)->keySequence(), spec.sequence);
  }
}

void TestShortcutsDialog::applyPersistsOnlyChangedKeys() {
  const QString description = Shortcuts::defaults().first().description;
  QVERIFY(!description.isEmpty());

  {
    ShortcutsDialog dlg(shortcuts.get());
    editorAt(dlg, rowFor(dlg, description))
        ->setKeySequence(QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));
    dlg.accept();
  }

  const auto stored = global->shortcuts();
  QCOMPARE(stored.size(), 1);
  QCOMPARE(stored.value(Shortcuts::defaults().first().key), QString("Ctrl+Alt+Z"));
  QCOMPARE(shortcuts->specs().first().sequence,
           QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));
}

MPZ_GUI_TEST_MAIN(TestShortcutsDialog)
#include "tst_shortcutsdialog.moc"
