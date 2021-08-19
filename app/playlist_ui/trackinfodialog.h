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
  void on_toolButtonOpenFileManager_clicked();
  void on_search(const QPoint &pos);

private:
  Ui::TrackInfoDialog *ui;

  QStandardItemModel model;
  QString track_dir;

  void setup_table(const Track &track);
  void setup_context_menu();

  void add_table_row(const QString& title, const QString &content);
};

#endif // TRACKINFODIALOG_H
