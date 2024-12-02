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
  void on_labelCoverArt_customContextMenuRequested(const QPoint &pos);

private:
  Ui::TrackInfoDialog *ui;

  QStandardItemModel model;
  QString track_dir;
  QString cover_art_path;

  void setup_table(const Track &track);
  void setup_context_menu();
  void setup_cover_art(const Track &track);

  void add_table_row(const QString& title, const QString &content);
};

#endif // TRACKINFODIALOG_H
