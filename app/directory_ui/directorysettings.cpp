#include "directorysettings.h"
#include "ui_directorysettings.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <QApplication>

DirectorySettings::DirectorySettings(const QStringList &paths, ModusOperandi &modus, QWidget *parent) : QDialog(parent), ui(new Ui::DirectorySettings), modus_operandi(modus) {
  ui->setupUi(this);

  model.setStringList(paths);
  ui->listView->setModel(&model);
#ifndef ENABLE_MPD_SUPPORT
  ui->pushButtonAddMpd->hide();
#endif
  connect(ui->listView, &QListView::clicked, this, &DirectorySettings::on_itemSelected);
  ui->pushButtonTestMpd->hide();
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

void DirectorySettings::on_itemSelected(const QModelIndex &index) {
  auto text = index.data().toString();

  if (text.startsWith("mpd://")) {
    ui->pushButtonTestMpd->show();
  } else {
    ui->pushButtonTestMpd->hide();
  }
  ui->labelMpdResult->clear();
}


void DirectorySettings::on_pushButtonTestMpd_clicked() {
#ifdef ENABLE_MPD_SUPPORT
  //ui->labelMpdResult->setPixmap(QApplication::style()->standardPixmap(QStyle::SP_DialogApplyButton));
  //QApplication::style()->standardPixmap(QStyle::SP_MessageBoxCritical)

  QModelIndex index = ui->listView->currentIndex();
  if (!index.isValid()) {
    return;
  }
  QString path = index.data().toString();
  if (path.isEmpty()) {
    return;
  }
  auto result = modus_operandi.mpd_connection.probe(QUrl(path));
  if (result.first) {
    ui->labelMpdResult->setText(QString("%1: %2").arg(tr("Success")).arg(result.second));
  } else {
    ui->labelMpdResult->setText(QString("%1: %2").arg(tr("Failure")).arg(result.second));
  }
  modus_operandi.mpd_connection.destroy();
#endif
}

