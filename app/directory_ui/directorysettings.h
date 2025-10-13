#ifndef DIRECTORYSETTINGS_H
#define DIRECTORYSETTINGS_H

#include <QDialog>
#include <QStringList>
#include <QStringListModel>

namespace Ui {
  class DirectorySettings;
}

class DirectorySettings : public QDialog {
  Q_OBJECT

public:
  explicit DirectorySettings(const QStringList &paths, QWidget *parent = nullptr);
  ~DirectorySettings();

  QStringList libraryPaths() const;

private slots:
  void on_pushButtonAddFolder_clicked();
  void on_pushButtonAddMpd_clicked();
  void on_pushButtonRemove_clicked();

private:
  Ui::DirectorySettings *ui;
  QStringListModel model;
};

#endif // DIRECTORYSETTINGS_H
