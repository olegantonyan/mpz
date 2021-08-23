#include "shortcuts_ui/shortcutsdialog.h"
#include "ui_shortcutsdialog.h"

ShortcutsDialog::ShortcutsDialog(const Shortcuts *shortcuts, QWidget *parent) : QDialog(parent), ui(new Ui::ShortcutsDialog) {
  ui->setupUi(this);

  auto data = shortcuts->describe();
  ui->tableWidget->setRowCount(data.size());
  ui->tableWidget->setColumnCount(2);
  ui->tableWidget->verticalHeader()->hide();
  ui->tableWidget->horizontalHeader()->hide();
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

  for (int row = 0; row < data.size(); row++) {
    auto name = new QTableWidgetItem(data[row].first);
    auto shortcut = new QTableWidgetItem(data[row].second);
    name->setFlags(Qt::ItemIsEnabled);
    shortcut->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(row, 0, name);
    ui->tableWidget->setItem(row, 1, shortcut);
  }
}

ShortcutsDialog::~ShortcutsDialog() {
  delete ui;
}
