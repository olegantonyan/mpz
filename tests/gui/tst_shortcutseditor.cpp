#include "fixture.h"

#include "shortcuts.h"
#include "shortcuts_ui/shortcutseditor.h"

#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

class TestShortcutsEditor : public QObject {
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

  int rowFor(ShortcutsEditor &page, const QString &description) const;
  static QKeySequenceEdit *editorAt(ShortcutsEditor &page, int row);
};

void TestShortcutsEditor::initTestCase() {
  QVERIFY(config.init());
}

void TestShortcutsEditor::init() {
  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  shortcuts = std::make_unique<Shortcuts>(*global, *local, &host);
}

void TestShortcutsEditor::cleanup() {
  shortcuts.reset();
  local.reset();
  global.reset();
  QFile::remove(config.path() + "/global.yml");
}

int TestShortcutsEditor::rowFor(ShortcutsEditor &page, const QString &description) const {
  auto *table = page.findChild<QTableWidget *>(QStringLiteral("tableWidget"));
  for (int i = 0; i < table->rowCount(); i++) {
    if (table->item(i, 0)->text() == description) {
      return i;
    }
  }
  return -1;
}

QKeySequenceEdit *TestShortcutsEditor::editorAt(ShortcutsEditor &page, int row) {
  auto *table = page.findChild<QTableWidget *>(QStringLiteral("tableWidget"));
  return qobject_cast<QKeySequenceEdit *>(table->cellWidget(row, 1));
}

void TestShortcutsEditor::listsOnlyDescribedShortcuts() {
  ShortcutsEditor page(shortcuts.get());
  auto *table = page.findChild<QTableWidget *>(QStringLiteral("tableWidget"));
  QVERIFY(table != nullptr);

  int described = 0;
  for (const auto &spec : shortcuts->specs()) {
    if (!spec.description.isEmpty()) {
      described++;
    }
  }
  QCOMPARE(table->rowCount(), described);
  QVERIFY(described > 0);
  QVERIFY(editorAt(page, 0) != nullptr);
}

void TestShortcutsEditor::multiChordEntryIsTruncatedToTheFirstCombination() {
  ShortcutsEditor page(shortcuts.get());
  auto *editor = editorAt(page, 0);

  editor->setKeySequence(QKeySequence::fromString("Ctrl+K, Ctrl+L", QKeySequence::PortableText));

  QCOMPARE(editor->keySequence().count(), 1);
  QCOMPARE(editor->keySequence(), QKeySequence::fromString("Ctrl+K", QKeySequence::PortableText));
}

void TestShortcutsEditor::takingABoundKeyClearsTheOtherRow() {
  ShortcutsEditor page(shortcuts.get());
  auto *warning = page.findChild<QLabel *>(QStringLiteral("warningLabel"));
  auto *first = editorAt(page, 0);
  auto *second = editorAt(page, 1);
  const QKeySequence taken = first->keySequence();
  QVERIFY(!taken.isEmpty());

  second->setKeySequence(taken);

  QVERIFY(first->keySequence().isEmpty());
  QCOMPARE(second->keySequence(), taken);
  QVERIFY(warning->text().contains(taken.toString(QKeySequence::NativeText)));
}

void TestShortcutsEditor::restoreDefaultsDoesNotTripTheStealLogic() {
  ShortcutsEditor page(shortcuts.get());
  auto *warning = page.findChild<QLabel *>(QStringLiteral("warningLabel"));
  auto *buttons = page.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
  editorAt(page, 0)->clear();
  editorAt(page, 1)->setKeySequence(QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));

  buttons->button(QDialogButtonBox::RestoreDefaults)->click();

  QVERIFY(warning->text().isEmpty());
  for (const auto &spec : Shortcuts::defaults()) {
    if (spec.description.isEmpty()) {
      continue;
    }
    const int row = rowFor(page, spec.description);
    QVERIFY2(row >= 0, qPrintable(spec.description));
    QCOMPARE(editorAt(page, row)->keySequence(), spec.sequence);
  }
}

void TestShortcutsEditor::applyPersistsOnlyChangedKeys() {
  const QString description = Shortcuts::defaults().first().description;
  QVERIFY(!description.isEmpty());

  {
    ShortcutsEditor page(shortcuts.get());
    editorAt(page, rowFor(page, description))
        ->setKeySequence(QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));
    page.apply();
  }

  const auto stored = global->shortcuts();
  QCOMPARE(stored.size(), 1);
  QCOMPARE(stored.value(Shortcuts::defaults().first().key), QString("Ctrl+Alt+Z"));
  QCOMPARE(shortcuts->specs().first().sequence,
           QKeySequence::fromString("Ctrl+Alt+Z", QKeySequence::PortableText));
}

MPZ_GUI_TEST_MAIN(TestShortcutsEditor)
#include "tst_shortcutseditor.moc"
