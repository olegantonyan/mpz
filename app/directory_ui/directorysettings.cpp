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
#include <QStyledItemDelegate>

namespace {
  // Hide the password when rendering an mpd:// entry, while leaving the stored
  // URL (used for persistence and editing) intact.
  QString redactMpdPassword(const QString &path) {
    if (!path.startsWith("mpd://")) {
      return path;
    }
    QUrl url(path);
    if (url.password().isEmpty()) {
      return path;
    }
    url.setPassword("***");
    return url.toString();
  }

  class PathDelegate : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QString displayText(const QVariant &value, const QLocale &locale) const override {
      return QStyledItemDelegate::displayText(redactMpdPassword(value.toString()), locale);
    }
  };
}

DirectorySettings::DirectorySettings(const QStringList &paths, ModusOperandi &modus, QWidget *parent) : QDialog(parent), ui(new Ui::DirectorySettings), modus_operandi(modus) {
  ui->setupUi(this);

  model.setStringList(paths);
  ui->listView->setModel(&model);
  ui->listView->setItemDelegate(new PathDelegate(this));
  ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  connect(ui->listView, &QAbstractItemView::doubleClicked, this, &DirectorySettings::on_pushButtonEdit_clicked);
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


void DirectorySettings::on_pushButtonEdit_clicked() {
  auto idx = ui->listView->currentIndex();
  if (!idx.isValid()) {
    return;
  }
  auto list = model.stringList();
  const QString current = list.at(idx.row());

  QString replacement;
  if (current.startsWith("mpd://")) {
#ifdef ENABLE_MPD_SUPPORT
    DirectoryUi::AddMpdDialog dlg(modus_operandi.mpd_client);
    dlg.setUrl(current);
    if (dlg.exec() != QDialog::Accepted) {
      return;
    }
    replacement = dlg.url();
#else
    return; // cannot edit an mpd entry without mpd support
#endif
  } else {
    replacement = QFileDialog::getExistingDirectory(this, QString(), current);
    if (replacement.isEmpty()) {
      return;
    }
  }

  list[idx.row()] = replacement;
  model.setStringList(list);
  ui->listView->setCurrentIndex(model.index(idx.row()));
}

void DirectorySettings::on_pushButtonRemove_clicked() {
  auto current_index = ui->listView->currentIndex();
  if (!current_index.isValid() || current_index.row() >= model.stringList().size()) {
    return;
  }
  model.removeRows(current_index.row(), 1);
}

void DirectorySettings::moveCurrent(int delta) {
  auto idx = ui->listView->currentIndex();
  if (!idx.isValid()) {
    return;
  }
  int row = idx.row();
  int target = row + delta;
  auto list = model.stringList();
  if (target < 0 || target >= list.size()) {
    return;
  }
  list.swapItemsAt(row, target);
  model.setStringList(list);
  ui->listView->setCurrentIndex(model.index(target));
}

void DirectorySettings::on_pushButtonUp_clicked() {
  moveCurrent(-1);
}

void DirectorySettings::on_pushButtonDown_clicked() {
  moveCurrent(1);
}


