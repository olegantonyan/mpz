#ifndef SORTINGPRESETS_H
#define SORTINGPRESETS_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringListModel>

namespace Ui {
  class SortingPresets;
}

class SortingPresets : public QDialog {
  Q_OBJECT

public:
  explicit SortingPresets(const QList<QPair<QString, QString> > &presets, QWidget *parent = nullptr);
  ~SortingPresets();

  QList<QPair<QString, QString> > currentPresets() const;

signals:
  void triggeredSort(const QString &criteria);

private slots:
  void on_buttonTest_clicked();
  void on_buttonAdd_clicked();
  void on_buttonRename_clicked();
  void on_buttonRemove_clicked();

  void on_buttonHelp_clicked();

private:
  Ui::SortingPresets *ui;
  QStringListModel model;
  QList<QPair<QString, QString> > presets;

  void refreshModel();
  QStringList itemList(const QList<QPair<QString, QString> > &presets) const;
};

#endif // SORTINGPRESETS_H
