#include "directorysettings.h"
#include "ui_directorysettings.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>

DirectorySettings::DirectorySettings(const QStringList &paths, QWidget *parent) : QDialog(parent), ui(new Ui::DirectorySettings) {
  ui->setupUi(this);

  model.setStringList(paths);
  ui->listView->setModel(&model);
#ifndef ENABLE_MPD_SUPPORT
  ui->pushButtonAddMpd.hide();
#endif
}

DirectorySettings::~DirectorySettings() {
  delete ui;
}

QStringList DirectorySettings::libraryPaths() const {
  return model.stringList();
}

void DirectorySettings::on_pushButtonAddFolder_clicked() {
  auto dir = QFileDialog::getExistingDirectory(this);
  auto existing = model.stringList();
  existing.append(dir);
  model.setStringList(existing);
}

void DirectorySettings::on_pushButtonAddMpd_clicked() {
#ifndef ENABLE_MPD_SUPPORT
  return;
#endif
  bool ok = false;
  QString text = QInputDialog::getText(nullptr, tr("Enter MPD host:port"), tr("Enter MPD host:port"), QLineEdit::Normal, "localhost:6600", &ok);

  if (ok && !text.isEmpty()) {
    if (!text.startsWith("mpd://")) {
      text.prepend("mpd://");
    }
    auto existing = model.stringList();
    existing.append(text);
    model.setStringList(existing);
  }
}


void DirectorySettings::on_pushButtonRemove_clicked() {
  auto current_index = ui->listView->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }
  model.removeRows(current_index.row(), 1);
}

