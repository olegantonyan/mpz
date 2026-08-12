#ifndef DYNAMICRANGEDIALOG_H
#define DYNAMICRANGEDIALOG_H

#include "dynamic_range/scanner.h"
#include "track.h"

#include <QDialog>
#include <QHash>
#include <QVector>

namespace Ui {
  class DynamicRangeDialog;
}

class DynamicRangeDialog : public QDialog {
  Q_OBJECT
public:
  explicit DynamicRangeDialog(const QVector<Track> &tracks, QWidget *parent = nullptr);
  ~DynamicRangeDialog();

  void reject() override;

private slots:
  void on_buttonCopy_clicked();
  void on_buttonSaveAs_clicked();
  void on_buttonClose_clicked();

  void on_progress(qint64 done_us, qint64 total_us);
  void on_segmentDone(quint64 uid, const DynamicRange::Result &result);
  void on_finished(bool cancelled);

private:
  void updateStatus();
  QString buildLog() const;

  Ui::DynamicRangeDialog *ui;
  DynamicRange::Scanner *scanner = nullptr;
  QVector<Track> tracks;
  QHash<quint64, DynamicRange::Result> results;
  int done_count = 0;
  bool running = false;
};

#endif // DYNAMICRANGEDIALOG_H
