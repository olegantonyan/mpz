#include "directorysettings.h"
#include "ui_directorysettings.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "addmpddialog.h"
#endif

#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <QApplication>
#include <QUrl>

DirectorySettings::DirectorySettings(const QStringList &paths, ModusOperandi &modus, QWidget *parent) : QDialog(parent), ui(new Ui::DirectorySettings), modus_operandi(modus) {
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

void DirectorySettings::on_pushButtonAddFolder_clicked() {
  auto dir = QFileDialog::getExistingDirectory(this);
  auto existing = model.stringList();
  existing.append(dir);
  model.setStringList(existing);
}

void DirectorySettings::on_pushButtonAddMpd_clicked() {
#ifdef ENABLE_MPD_SUPPORT
  DirectoryUi::AddMpdDialog dlg(modus_operandi.mpd_client);
  if (dlg.exec() == QDialog::Accepted) {
    auto url = dlg.url();
    auto existing = model.stringList();
    existing.append(url);
    model.setStringList(existing);
  }
#endif
}


void DirectorySettings::on_pushButtonRemove_clicked() {
  auto current_index = ui->listView->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }
  model.removeRows(current_index.row(), 1);
}


