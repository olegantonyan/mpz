#ifndef TRACKINFODIALOG_H
#define TRACKINFODIALOG_H

#include "track.h"

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
  class TrackInfoDialog;
}

class TrackInfoDialog : public QDialog {
  Q_OBJECT

public:
  explicit TrackInfoDialog(const Track &track, QWidget *parent = nullptr);
  ~TrackInfoDialog();

private slots:
  void on_copy(const QPoint &pos);

private:
  Ui::TrackInfoDialog *ui;

  QStandardItemModel model;

  void setup_table(const Track &track);
  void setup_context_menu();
};

#endif // TRACKINFODIALOG_H
