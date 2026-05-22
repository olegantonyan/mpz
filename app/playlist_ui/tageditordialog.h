#ifndef TAGEDITORDIALOG_H
#define TAGEDITORDIALOG_H

#include "track.h"
#include "playlist/playlist.h"

#include <QDialog>
#include <QList>
#include <QString>
#include <QVector>
#include <memory>

namespace Ui {
  class TagEditorDialog;
}

class TagEditorDialog : public QDialog {
  Q_OBJECT

public:
  explicit TagEditorDialog(const QVector<Track> &tracks,
                           std::shared_ptr<Playlist::Playlist> playlist = nullptr,
                           QWidget *parent = nullptr);
  ~TagEditorDialog();

signals:
  void saved(const QList<quint64> &uids);

private slots:
  void on_save();
  void on_prevTrack();
  void on_nextTrack();

private:
  struct FieldState {
    bool dirty = false;
  };

  Ui::TagEditorDialog *ui;
  QVector<Track> _tracks;
  std::shared_ptr<Playlist::Playlist> _playlist;

  FieldState _artist;
  FieldState _album;
  FieldState _title;
  FieldState _year;
  FieldState _track_number;
  FieldState _genre;
  FieldState _comment;

  void populate_fields();
  void update_header();
  void update_nav_state();
  void clear_dirty();
  void load_track(int delta);
  bool commit_dirty();
  bool save_one(const Track &t, QString *error) const;
};

#endif // TAGEDITORDIALOG_H
