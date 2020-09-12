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

private:
  Ui::TrackInfoDialog *ui;

  QStandardItemModel model;

  const Track &track;
};

#endif // TRACKINFODIALOG_H
