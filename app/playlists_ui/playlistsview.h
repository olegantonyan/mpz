#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"
#include "playlist.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QPoint>
#include <QDir>
#include <QModelIndex>

namespace PlaylistsUi {
  class View : public QObject {
    Q_OBJECT

  public:
    explicit View(QListView *view, QObject *parent = nullptr);

  public slots:
    void on_createPlaylist(const QDir &filepath);

  signals:
    void selected(const Playlist &item);
    void emptied();

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);
    void on_itemActivated(const QModelIndex &index);

  private:
    QListView *view;
    Model *model;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
