#ifndef TRACKINFODIALOG_H
#define TRACKINFODIALOG_H

#include "track.h"
#include "playlist/playlist.h"
#include "config/global.h"

#include <QDialog>
#include <QPixmap>
#include <QStandardItemModel>
#include <QTableView>
#include <memory>

namespace Ui {
  class TrackInfoDialog;
}

class TagEditorDialog;

class TrackInfoDialog : public QDialog {
  Q_OBJECT

public:
  explicit TrackInfoDialog(const Track &track,
                           Config::Global &global,
                           std::shared_ptr<Playlist::Playlist> playlist = nullptr,
                           QWidget *parent = nullptr);
  ~TrackInfoDialog();

signals:
  void tagEditorOpened(TagEditorDialog *editor);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void on_toolButtonOpenFileManager_clicked();
  void on_toolButtonEditTags_clicked();
  void on_labelCoverArt_customContextMenuRequested(const QPoint &pos);
  void refresh_track(const QList<quint64> &uids);

private:
  Ui::TrackInfoDialog *ui;

  QStandardItemModel model;
  QStandardItemModel model_tags;
  QStandardItemModel model_file;
  QStandardItemModel model_other;
  QPixmap cover_art;
  Track _track;
  Config::Global &global_conf;
  std::shared_ptr<Playlist::Playlist> _playlist;
  QString base_title;
  QString track_path;
  QString cover_art_path;
  QStringList tab_titles;

  void setup_table();
  void setup_view(QTableView *view, QStandardItemModel *m);
  void setup_context_menu(QTableView *view);
  void setup_cover_art();
  void render_cover_art();
  void setup_lyrics();

  void add_general_rows();
  void add_tags_rows();
  void add_file_rows();
  void add_other_rows();

  void add_table_row(QStandardItemModel &m, const QString &title, const QString &content);

  void rescale_cover_art();

  QString yes_no(bool v) const;
  QString value_at(const QTableView *view, const QPoint &pos) const;

  QString fetch_embedded_lyrics() const;
  QString fetch_sidecar_lyrics() const;
  void render_lyrics(const QString &source, const QString &raw);
  void render_lyrics_state(const QString &message);
};

#endif // TRACKINFODIALOG_H
