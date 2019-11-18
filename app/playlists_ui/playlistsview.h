#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"
#include "playlist.h"
#include "config/local.h"
#include "track.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QPoint>
#include <QDir>
#include <QModelIndex>
#include <memory>

namespace PlaylistsUi {
  class View : public QObject {
    Q_OBJECT

  public:
    explicit View(QListView *view, Config::Local &conf, QObject *parent = nullptr);
    void load();
    std::shared_ptr<Playlist> playlistByTrackUid(quint64 track_uid) const;

  public slots:
    void on_createPlaylist(const QDir &filepath);
    void on_appendToCurrentPlaylist(const QDir &filepath);
    void on_jumpTo(const std::shared_ptr<Playlist> playlist);

  signals:
    void selected(const std::shared_ptr<Playlist> item);
    void emptied();

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);
    void on_itemActivated(const QModelIndex &index);

  private:
    QListView *view;
    Model *model;

    Config::Local &local_conf;
    void persist(int current_index);
  };
}

#endif // PLAYLISTSVIEWMODEL_H
