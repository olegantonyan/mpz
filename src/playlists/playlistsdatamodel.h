#ifndef PLAYLISTSDATAMODEL_H
#define PLAYLISTSDATAMODEL_H

#include "playlistitem.h"

#include <QAbstractListModel>
#include <QList>

namespace Playlists {
  class PlaylistsDataModel : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit PlaylistsDataModel(QObject *parent = nullptr);

    void append(const Playlists::PlaylistItem &item);

    // Header:
    //QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    //bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /*// Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
*/
    //Qt::ItemFlags flags(const QModelIndex &index) const override;
  private:
    QList<Playlists::PlaylistItem> list;
  };
}

#endif // PLAYLISTSDATAMODEL_H
