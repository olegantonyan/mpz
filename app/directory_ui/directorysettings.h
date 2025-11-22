#ifndef DIRECTORYSETTINGS_H
#define DIRECTORYSETTINGS_H

#include "modusoperandi.h"

#include <QDialog>
#include <QStringList>
#include <QStringListModel>

namespace Ui {
  class DirectorySettings;
}

class DirectorySettings : public QDialog {
  Q_OBJECT

public:
  explicit DirectorySettings(const QStringList &paths, ModusOperandi &modus, QWidget *parent = nullptr);
  ~DirectorySettings();

  QStringList libraryPaths() const;

private slots:
  void on_pushButtonAddFolder_clicked();
  void on_pushButtonAddMpd_clicked();
  void on_pushButtonRemove_clicked();
  void on_itemSelected(const QModelIndex &index);

  void on_pushButtonTestMpd_clicked();

private:
  Ui::DirectorySettings *ui;
  QStringListModel model;
  ModusOperandi &modus_operandi;
};

#endif // DIRECTORYSETTINGS_H
