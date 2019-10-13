#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"
#include "playlist.h"
#include "config/local.h"

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

  public slots:
    void on_createPlaylist(const QDir &filepath);

  signals:
    void selected(const std::shared_ptr<Playlist> item);
    void emptied();

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);
    void on_itemActivated(const QModelIndex &index);

  private:
    QListView *view;
    Model *model;
    std::shared_ptr<Playlist> current;
    Config::Local &local_conf;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
