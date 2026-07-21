#include "radiostationsdialog.h"
#include "radiostationdialog.h"
#include "radio/catalog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QUuid>
#include <QVBoxLayout>

namespace DirectoryUi {
  RadioStationsDialog::RadioStationsDialog(const QVector<Radio::Station> &stations, QWidget *parent) :
    QDialog(parent), _stations(stations) {
    setWindowTitle(tr("Radio stations"));

    table = new QTableWidget(0, 3);
    table->setHorizontalHeaderLabels({tr("Name"), tr("Group"), tr("Stream URL")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    connect(table, &QTableWidget::doubleClicked, this, &RadioStationsDialog::editStation);

    auto *add = new QPushButton(tr("Add..."));
    auto *edit = new QPushButton(tr("Edit..."));
    auto *remove = new QPushButton(tr("Remove"));
    auto *defaults = new QPushButton(tr("Restore defaults"));
    connect(add, &QPushButton::clicked, this, &RadioStationsDialog::addStation);
    connect(edit, &QPushButton::clicked, this, &RadioStationsDialog::editStation);
    connect(remove, &QPushButton::clicked, this, &RadioStationsDialog::removeStation);
    connect(defaults, &QPushButton::clicked, this, &RadioStationsDialog::restoreDefaults);

    auto *buttons_col = new QVBoxLayout;
    buttons_col->addWidget(add);
    buttons_col->addWidget(edit);
    buttons_col->addWidget(remove);
    buttons_col->addSpacing(12);
    buttons_col->addWidget(defaults);
    buttons_col->addStretch();

    auto *top = new QHBoxLayout;
    top->addWidget(table);
    top->addLayout(buttons_col);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &RadioStationsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &RadioStationsDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(buttons);
    resize(1000, 600);

    refreshTable();
  }

  void RadioStationsDialog::refreshTable() {
    table->setRowCount(_stations.size());
    for (int i = 0; i < _stations.size(); i++) {
      const auto &s = _stations.at(i);
      table->setItem(i, 0, new QTableWidgetItem(s.name));
      table->setItem(i, 1, new QTableWidgetItem(s.group));
      table->setItem(i, 2, new QTableWidgetItem(s.url));
    }
    table->resizeColumnsToContents();
  }

  int RadioStationsDialog::currentRow() const {
    const auto rows = table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
  }

  bool RadioStationsDialog::hasId(const QString &id, int except_row) const {
    for (int i = 0; i < _stations.size(); i++) {
      if (i != except_row && _stations.at(i).id == id) {
        return true;
      }
    }
    return false;
  }

  void RadioStationsDialog::addStation() {
    RadioStationDialog dlg(Radio::Station{}, this);
    if (dlg.exec() != QDialog::Accepted) {
      return;
    }
    auto station = dlg.station();
    while (hasId(station.id, -1)) {
      station.id = QStringLiteral("user-") + QUuid::createUuid().toString(QUuid::Id128);
    }
    _stations << station;
    refreshTable();
    table->selectRow(_stations.size() - 1);
  }

  void RadioStationsDialog::editStation() {
    const int row = currentRow();
    if (row < 0) {
      return;
    }
    RadioStationDialog dlg(_stations.at(row), this);
    if (dlg.exec() != QDialog::Accepted) {
      return;
    }
    _stations[row] = dlg.station();
    refreshTable();
    table->selectRow(row);
  }

  void RadioStationsDialog::removeStation() {
    const int row = currentRow();
    if (row < 0) {
      return;
    }
    _stations.remove(row);
    refreshTable();
  }

  void RadioStationsDialog::restoreDefaults() {
    const auto answer = QMessageBox::question(
      this, windowTitle(), tr("Replace the list with the built-in stations?"));
    if (answer == QMessageBox::Yes) {
      _stations = Radio::Catalog::builtin();
      refreshTable();
    }
  }

  QVector<Radio::Station> RadioStationsDialog::stations() const {
    return _stations;
  }
}
