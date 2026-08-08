#include "shortcuts_ui/shortcutsdialog.h"
#include "ui_shortcutsdialog.h"

#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QSignalBlocker>
#include <QMap>

ShortcutsDialog::ShortcutsDialog(Shortcuts *sc, QWidget *parent) : QDialog(parent), ui(new Ui::ShortcutsDialog), shortcuts(sc) {
  ui->setupUi(this);

  const auto editor_hint = QKeySequenceEdit().sizeHint();

  ui->tableWidget->setColumnCount(2);
  ui->tableWidget->verticalHeader()->hide();
  ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  ui->tableWidget->verticalHeader()->setDefaultSectionSize(editor_hint.height());
  ui->tableWidget->horizontalHeader()->hide();
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
  ui->tableWidget->setColumnWidth(1, editor_hint.width());
  ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection);

  for (const auto &spec : shortcuts->specs()) {
    if (spec.description.isEmpty()) {
      continue;
    }
    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    auto *name = new QTableWidgetItem(spec.description);
    name->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(row, 0, name);

    auto *editor = new QKeySequenceEdit(spec.sequence, ui->tableWidget);
    connect(editor, &QKeySequenceEdit::keySequenceChanged, this, [this, row](const QKeySequence &seq) {
      onSequenceChanged(row, seq);
    });
    ui->tableWidget->setCellWidget(row, 1, editor);

    row_keys << spec.key;
    row_editors << editor;
  }

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ShortcutsDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ShortcutsDialog::reject);
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &ShortcutsDialog::onButtonBoxClicked);
  connect(this, &QDialog::accepted, this, &ShortcutsDialog::apply);
}

ShortcutsDialog::~ShortcutsDialog() {
  delete ui;
}

void ShortcutsDialog::onSequenceChanged(int row, const QKeySequence &sequence) {
  // keep one combination; setMaximumSequenceLength is Qt 6.5+
  if (sequence.count() > 1) {
    auto first = sequence.toString(QKeySequence::PortableText).section(", ", 0, 0);
    row_editors[row]->setKeySequence(QKeySequence::fromString(first, QKeySequence::PortableText));
    return;
  }

  ui->warningLabel->clear();
  if (sequence.isEmpty()) {
    return;
  }

  for (int other = 0; other < row_editors.size(); other++) {
    if (other == row || row_editors[other]->keySequence() != sequence) {
      continue;
    }
    const QString taken_from = ui->tableWidget->item(other, 0)->text();
    row_editors[other]->clear();
    ui->warningLabel->setText(tr("%1 taken from \"%2\"")
                              .arg(sequence.toString(QKeySequence::NativeText), taken_from));
  }
}

void ShortcutsDialog::onButtonBoxClicked(QAbstractButton *btn) {
  if (ui->buttonBox->buttonRole(btn) == QDialogButtonBox::ResetRole) {
    restoreDefaults();
  }
}

void ShortcutsDialog::restoreDefaults() {
  for (const auto &spec : Shortcuts::defaults()) {
    const int row = row_keys.indexOf(spec.key);
    if (row < 0) {
      continue;
    }
    // a bulk fill passes through states where two rows hold the same key
    QSignalBlocker blocker(row_editors[row]);
    row_editors[row]->setKeySequence(spec.sequence);
  }
  ui->warningLabel->clear();
}

void ShortcutsDialog::apply() {
  QMap<QString, QString> overrides;
  for (const auto &spec : Shortcuts::defaults()) {
    const int row = row_keys.indexOf(spec.key);
    if (row < 0) {
      continue;
    }
    const auto seq = row_editors[row]->keySequence();
    if (seq != spec.sequence) {
      overrides.insert(spec.key, seq.toString(QKeySequence::PortableText));
    }
  }
  shortcuts->applyOverrides(overrides);
}
