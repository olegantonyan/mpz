#ifndef SORTINGPRESETS_H
#define SORTINGPRESETS_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringListModel>

typedef QPair<QString, QString> SortingPreset;

namespace Ui {
  class SortingPresets;
}

class SortingPresetsDialog : public QDialog {
  Q_OBJECT

public:
  explicit SortingPresetsDialog(const QList<SortingPreset> &presets, QWidget *parent = nullptr);
  ~SortingPresetsDialog();

  QList<SortingPreset> currentPresets() const;

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
  QList<SortingPreset> presets;

  void refreshModel();
  QStringList itemList(const QList<SortingPreset> &presets) const;
};

#endif // SORTINGPRESETS_H
