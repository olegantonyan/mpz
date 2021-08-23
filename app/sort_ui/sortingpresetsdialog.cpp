#include "sortingpresetsdialog.h"
#include "ui_sortingpresets.h"
#include "playlist/sorter.h"

#include <QInputDialog>
#include <QStyle>
#include <QMessageBox>

SortingPresetsDialog::SortingPresetsDialog(const QList<SortingPreset> &_pr, QWidget *parent) : QDialog(parent), ui(new Ui::SortingPresets), presets(_pr) {
  ui->setupUi(this);
  ui->listViewPresets->setModel(&model);
  ui->buttonHelp->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
  refreshModel();
  ui->labelDefault->setText(tr("Default") + ": " + Playlist::Sorter::defaultCriteria());
}

SortingPresetsDialog::~SortingPresetsDialog() {
  delete ui;
}

QList<SortingPreset> SortingPresetsDialog::currentPresets() const {
  return presets;
}

void SortingPresetsDialog::on_buttonTest_clicked() {
  //TODO validate format
  QString value = ui->lineEditNewPreset->text();
  if (value.isEmpty()) {
    return;
  }
  emit triggeredSort(value);
}

void SortingPresetsDialog::on_buttonAdd_clicked() {
  QString value = ui->lineEditNewPreset->text();
  if (value.isEmpty()) {
    return;
  }
  //TODO validate format
  SortingPreset item("", value);
  presets << item;
  refreshModel();
}

void SortingPresetsDialog::on_buttonRename_clicked() {
  auto current_index = ui->listViewPresets->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }

  bool ok;
  QString name = QInputDialog::getText(this, tr("Rename sorting preset"), "", QLineEdit::Normal, presets.at(current_index.row()).first, &ok, Qt::Widget);
  if (ok && !name.isEmpty()) {
    SortingPreset item(name, presets.at(current_index.row()).second);
    presets.replace(current_index.row(), item);
    refreshModel();
  }
}

void SortingPresetsDialog::on_buttonRemove_clicked() {
  auto current_index = ui->listViewPresets->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }
  presets.removeAt(current_index.row());
  refreshModel();
}

void SortingPresetsDialog::refreshModel() {
  model.setStringList(itemList(presets));
}

QStringList SortingPresetsDialog::itemList(const QList<SortingPreset> &presets) const {
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

void SortingPresetsDialog::on_buttonHelp_clicked() {
  static const QString HELP_TEXT = (
        "Fields available to sort (case insensitive):<br />"
        " * Artist<br />"
        " * Album<br />"
        " * Title<br />"
        " * Year<br />"
        " * Filename<br />"
        " * TrackNumber<br />"
        " * Directory</br />"
        "<br />"
        "By default sort in ascending order. Add - before the field name to change to descending order.<br />"
        "Use / to build nested multilevel sorting criteria. Examples:<br />"
        " * Artist / -Year / Title : sort first by artist, then by year in descending order, then by title<br />"
        " * -Title : sort only by title in descending order<br />"
        );

  QMessageBox *msg = new QMessageBox;
  msg->setWindowTitle(tr("Sorting presets description"));
  msg->setModal(false);
  msg->setTextInteractionFlags(Qt::TextBrowserInteraction);
  msg->setTextFormat(Qt::RichText);
  connect(msg, &QMessageBox::finished, msg, &QMessageBox::deleteLater);
  msg->setText(HELP_TEXT);
  msg->show();
}
