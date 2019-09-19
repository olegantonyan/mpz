#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QAbstractItemModel>
#include <QPoint>
#include <QDir>

namespace Playlists {
  class View : public QObject {
    Q_OBJECT

  public:
    explicit View(QListView *view, QObject *parent = nullptr);

  public slots:
    void on_createPlaylist(const QDir &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);

  private:
    QListView *view;
    Playlists::Model *model;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
