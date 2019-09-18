#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsdatamodel.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QAbstractItemModel>
#include <QPoint>
#include <QDir>

namespace Playlists {
  class PlaylistsViewModel : public QObject {
    Q_OBJECT

  public:
    explicit PlaylistsViewModel(QListView *view, QObject *parent = nullptr);

  public slots:
    void on_createPlaylist(const QDir &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);

  private:
    QListView *view;
    Playlists::PlaylistsDataModel *model;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
