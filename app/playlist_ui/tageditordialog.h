#ifndef TAGEDITORDIALOG_H
#define TAGEDITORDIALOG_H

#include "track.h"

#include <QDialog>
#include <QList>
#include <QString>
#include <QVector>

namespace Ui {
  class TagEditorDialog;
}

class TagEditorDialog : public QDialog {
  Q_OBJECT

public:
  explicit TagEditorDialog(const QVector<Track> &tracks, QWidget *parent = nullptr);
  ~TagEditorDialog();

signals:
  void saved(const QList<quint64> &uids);

private slots:
  void on_save();

private:
  struct FieldState {
    bool dirty = false;
  };

  Ui::TagEditorDialog *ui;
  QVector<Track> _tracks;

  FieldState _artist;
  FieldState _album;
  FieldState _title;
  FieldState _year;
  FieldState _track_number;
  FieldState _genre;
  FieldState _comment;

  void populate_fields();
  bool save_one(const Track &t, QString *error) const;
};

#endif // TAGEDITORDIALOG_H
