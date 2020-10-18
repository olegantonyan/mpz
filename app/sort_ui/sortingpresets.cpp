#include "sortingpresets.h"
#include "ui_sortingpresets.h"

#include <QInputDialog>
#include <QStyle>
#include <QMessageBox>

SortingPresets::SortingPresets(const QList<QPair<QString, QString> > &_pr, QWidget *parent) : QDialog(parent), ui(new Ui::SortingPresets), presets(_pr) {
  ui->setupUi(this);
  ui->listViewPresets->setModel(&model);
  ui->buttonHelp->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
  refreshModel();
}

SortingPresets::~SortingPresets() {
  delete ui;
}

QList<QPair<QString, QString> > SortingPresets::currentPresets() const {
  return presets;
}

void SortingPresets::on_buttonTest_clicked() {
  //TODO validate format
  QString value = ui->lineEditNewPreset->text();
  if (value.isEmpty()) {
    return;
  }
  emit triggeredSort(value);
}

void SortingPresets::on_buttonAdd_clicked() {
  QString value = ui->lineEditNewPreset->text();
  if (value.isEmpty()) {
    return;
  }
  //TODO validate format
  QPair<QString, QString> item("", value);
  presets << item;
  refreshModel();
}

void SortingPresets::on_buttonRename_clicked() {
  auto current_index = ui->listViewPresets->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }

  bool ok;
  QString name = QInputDialog::getText(this, tr("Rename sorting preset"), "", QLineEdit::Normal, presets.at(current_index.row()).first, &ok, Qt::Widget);
  if (ok && !name.isEmpty()) {
    QPair<QString, QString> item(name, presets.at(current_index.row()).second);
    presets.replace(current_index.row(), item);
    refreshModel();
  }
}

void SortingPresets::on_buttonRemove_clicked() {
  auto current_index = ui->listViewPresets->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }
  presets.removeAt(current_index.row());
  refreshModel();
}

void SortingPresets::refreshModel() {
  model.setStringList(itemList(presets));
}

QStringList SortingPresets::itemList(const QList<QPair<QString, QString> > &presets) const {
  QStringList list;
  for (auto i : presets) {
    if (i.first.isEmpty() || i.first == i.second) {
      list << i.second;
    } else {
      list << i.second + " (" + i.first + ")";
    }
  }
  return list;
}

void SortingPresets::on_buttonHelp_clicked() {
  QMessageBox *msg = new QMessageBox;
  msg->setWindowTitle(tr("Sorting presets description"));
  msg->setModal(false);
  connect(msg, &QMessageBox::finished, msg, &QMessageBox::deleteLater);
  msg->setText("hello");
  msg->show();
}
