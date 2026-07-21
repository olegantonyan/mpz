#include "directorysettings.h"
#include "ui_directorysettings.h"
#include "radiolibrary.h"
#include "radiostationsdialog.h"
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
  class PathDelegate : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QString displayText(const QVariant &value, const QLocale &locale) const override {
      return QStyledItemDelegate::displayText(
        DirectoryUi::libraryPathLabel(value.toString()), locale);
    }
  };
}

DirectorySettings::DirectorySettings(const QStringList &paths, ModusOperandi &modus, Config::Global &global_cfg, QWidget *parent) : QDialog(parent), ui(new Ui::DirectorySettings), modus_operandi(modus), global_conf(global_cfg) {
  ui->setupUi(this);

  model.setStringList(paths);
  ui->listView->setModel(&model);
  ui->listView->setItemDelegate(new PathDelegate(this));
  ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  connect(ui->listView, &QAbstractItemView::doubleClicked, this, &DirectorySettings::on_pushButtonEdit_clicked);
  connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this]() { updateMoveButtons(); });
  updateMoveButtons();
}

DirectorySettings::~DirectorySettings() {
  delete ui;
}

QStringList DirectorySettings::libraryPaths() const {
  return model.stringList();
}

void DirectorySettings::on_pushButtonAddFolder_clicked() {
  auto dir = QFileDialog::getExistingDirectory(this);
  if (dir.isEmpty()) {
    return;
  }
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


void DirectorySettings::on_pushButtonRadioStations_clicked() {
  editRadioStations();
}

bool DirectorySettings::radioStationsEdited() const {
  return stations_edited;
}

void DirectorySettings::editRadioStations() {
  DirectoryUi::RadioStationsDialog dlg(global_conf.radioStations(), this);
  if (dlg.exec() == QDialog::Accepted) {
    global_conf.saveRadioStations(dlg.stations());
    global_conf.sync();
    stations_edited = true;
  }
}

void DirectorySettings::on_pushButtonEdit_clicked() {
  auto idx = ui->listView->currentIndex();
  if (!idx.isValid()) {
    return;
  }
  auto list = model.stringList();
  const QString current = list.at(idx.row());

  if (DirectoryUi::isRadioLibraryPath(current)) {
    editRadioStations();
    return;
  }

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
  if (DirectoryUi::isRadioLibraryPath(model.stringList().at(current_index.row()))) {
    return; // the radio entry is permanent
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
  list.move(row, target);
  model.setStringList(list);
  ui->listView->setCurrentIndex(model.index(target));
}

void DirectorySettings::updateMoveButtons() {
  auto idx = ui->listView->currentIndex();
  ui->pushButtonUp->setEnabled(idx.isValid() && idx.row() > 0);
  ui->pushButtonDown->setEnabled(idx.isValid() && idx.row() < model.rowCount() - 1);

  const bool is_radio = idx.isValid()
    && DirectoryUi::isRadioLibraryPath(model.stringList().at(idx.row()));
  ui->pushButtonRemove->setEnabled(idx.isValid() && !is_radio);
}

void DirectorySettings::on_pushButtonUp_clicked() {
  moveCurrent(-1);
}

void DirectorySettings::on_pushButtonDown_clicked() {
  moveCurrent(1);
}

