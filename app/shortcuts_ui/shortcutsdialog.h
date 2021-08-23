#ifndef SHORTCUTSDIALOG_H
#define SHORTCUTSDIALOG_H

#include "shortcuts.h"

#include <QDialog>

namespace Ui {
  class ShortcutsDialog;
}

class ShortcutsDialog : public QDialog {
  Q_OBJECT

public:
  explicit ShortcutsDialog(const Shortcuts *shortcuts, QWidget *parent = nullptr);
  ~ShortcutsDialog();

private:
  Ui::ShortcutsDialog *ui;
};

#endif // SHORTCUTSDIALOG_H
