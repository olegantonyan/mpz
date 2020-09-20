#ifndef PLAYBACKLOGDIALOG_H
#define PLAYBACKLOGDIALOG_H

#include <QDialog>
#include <QPoint>

#include "playback_log_ui/playbackloguimodel.h"

namespace Ui {
  class PlaybackLogDialog;
}

class PlaybackLogDialog : public QDialog {
  Q_OBJECT
public:
  explicit PlaybackLogDialog(PlaybackLogUi::Model *model, QWidget *parent = nullptr);
  ~PlaybackLogDialog();

signals:
  void jumpToTrack(quint64 track_uid);

private slots:
  void on_copy(const QPoint &pos);
  void on_jumpTo(const QModelIndex &idx);

private:
  Ui::PlaybackLogDialog *ui;
  PlaybackLogUi::Model *model;

  void setup_context_menu();
};

#endif // PLAYBACKLOGDIALOG_H
