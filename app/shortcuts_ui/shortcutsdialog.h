#ifndef SHORTCUTSDIALOG_H
#define SHORTCUTSDIALOG_H

#include "shortcuts.h"

#include <QDialog>
#include <QVector>
#include <QString>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QAbstractButton>

namespace Ui {
  class ShortcutsDialog;
}

class ShortcutsDialog : public QDialog {
  Q_OBJECT

public:
  explicit ShortcutsDialog(Shortcuts *shortcuts, QWidget *parent = nullptr);
  ~ShortcutsDialog();

private:
  Ui::ShortcutsDialog *ui;
  Shortcuts *shortcuts;

  QVector<QString> row_keys;
  QVector<QKeySequenceEdit *> row_editors;

  void onSequenceChanged(int row, const QKeySequence &sequence);
  void onButtonBoxClicked(QAbstractButton *btn);
  void restoreDefaults();
  void apply();
};

#endif // SHORTCUTSDIALOG_H
