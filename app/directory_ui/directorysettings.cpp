#include "directorysettings.h"
#include "ui_directorysettings.h"

#include <QFileDialog>
#include <QDebug>

DirectorySettings::DirectorySettings(const QStringList &paths, QWidget *parent) : QDialog(parent), ui(new Ui::DirectorySettings) {
  ui->setupUi(this);

  model.setStringList(paths);
  ui->listView->setModel(&model);
}

DirectorySettings::~DirectorySettings() {
  delete ui;
}

QStringList DirectorySettings::libraryPaths() const {
  return model.stringList();
}

void DirectorySettings::on_pushButtonAdd_clicked() {
  auto dir = QFileDialog::getExistingDirectory(this);
  auto existing = model.stringList();
  existing.append(dir);
  model.setStringList(existing);
}

void DirectorySettings::on_pushButtonRemove_clicked() {
  auto current_index = ui->listView->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }
  model.removeRows(current_index.row(), 1);
}
