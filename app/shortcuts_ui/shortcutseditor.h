#ifndef SHORTCUTSEDITOR_H
#define SHORTCUTSEDITOR_H

#include "shortcuts.h"

#include <QWidget>
#include <QVector>
#include <QString>
#include <QKeySequence>
#include <QKeySequenceEdit>

namespace Ui {
  class ShortcutsEditor;
}

class ShortcutsEditor : public QWidget {
  Q_OBJECT

public:
  explicit ShortcutsEditor(Shortcuts *shortcuts, QWidget *parent = nullptr);
  ~ShortcutsEditor();

  void apply();

private:
  Ui::ShortcutsEditor *ui;
  Shortcuts *shortcuts;

  QVector<QString> row_keys;
  QVector<QKeySequenceEdit *> row_editors;

  void onSequenceChanged(int row, const QKeySequence &sequence);
  void restoreDefaults();
};

#endif // SHORTCUTSEDITOR_H
