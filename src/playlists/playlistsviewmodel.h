#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsdatamodel.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QAbstractItemModel>

namespace Playlists {
  class PlaylistsViewModel : public QObject {
    Q_OBJECT

  public:
    explicit PlaylistsViewModel(QListView *view, QObject *parent = nullptr);

  public slots:
    void on_createPlaylist(const QString &filepath);

  private:
    QListView *view;
    Playlists::PlaylistsDataModel *model;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
