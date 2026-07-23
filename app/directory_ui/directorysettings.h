#ifndef DIRECTORYSETTINGS_H
#define DIRECTORYSETTINGS_H

#include "modusoperandi.h"
#include "config/global.h"

#include <QDialog>
#include <QStringList>
#include <QStringListModel>

namespace Ui {
  class DirectorySettings;
}

class DirectorySettings : public QDialog {
  Q_OBJECT

public:
  explicit DirectorySettings(const QStringList &paths, ModusOperandi &modus, Config::Global &global_cfg, QWidget *parent = nullptr);
  ~DirectorySettings();

  QStringList libraryPaths() const;
  bool radioStationsEdited() const;

private slots:
  void on_pushButtonAddFolder_clicked();
  void on_pushButtonAddMpd_clicked();
  void on_pushButtonRadioStations_clicked();
  void on_pushButtonEdit_clicked();
  void on_pushButtonRemove_clicked();
  void on_pushButtonUp_clicked();
  void on_pushButtonDown_clicked();

private:
  void moveCurrent(int delta);
  void updateMoveButtons();
  void editRadioStations();

  Ui::DirectorySettings *ui;
  QStringListModel model;
  ModusOperandi &modus_operandi;
  Config::Global &global_conf;
  bool stations_edited = false;
};

#endif // DIRECTORYSETTINGS_H
