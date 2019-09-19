#ifndef PLAYLISTSDATAMODEL_H
#define PLAYLISTSDATAMODEL_H

#include "playlistitem.h"

#include <QAbstractListModel>
#include <QList>

namespace Playlists {
  class Model : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    void append(const Playlists::PlaylistItem &item);
    void remove(const QModelIndex &index);
    PlaylistItem itemAt(const QModelIndex &index) const;
    PlaylistItem repalceAt(const QModelIndex &index, const Playlists::PlaylistItem &newItem);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  private:
    QList<Playlists::PlaylistItem> list;
  };
}

#endif // PLAYLISTSDATAMODEL_H
