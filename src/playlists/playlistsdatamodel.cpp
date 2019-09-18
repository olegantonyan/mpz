#include "playlistsdatamodel.h"

#include <QDebug>

namespace Playlists {
  PlaylistsDataModel::PlaylistsDataModel(QObject *parent) : QAbstractListModel(parent) {
  }

  void PlaylistsDataModel::append(const PlaylistItem &item) {
    list.append(item);
    QModelIndex idx = createIndex(list.size() - 1, 0);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
  }

  /*QVariant PlaylistsDataModel::headerData(int section, Qt::Orientation orientation, int role) const {
    return QVariant();
  }

  bool PlaylistsDataModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) {
    if (value != headerData(section, orientation, role)) {
      // FIXME: Implement me!
      emit headerDataChanged(orientation, section, section);
      return true;
    }
    return false;
  }*/

  int PlaylistsDataModel::rowCount(const QModelIndex &parent) const {
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid()) {
      return 0;
    }

    qDebug() << list.size();

    return list.size();
  }

  QVariant PlaylistsDataModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    if (role == Qt::DisplayRole) {
      if (list.size() > index.row()) {
        return list.at(index.row()).getPath();
      } else {
        return QVariant();
      }
    }

    return QVariant();
  }
/*
  bool PlaylistsDataModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (data(index, role) != value) {
      // FIXME: Implement me!
      emit dataChanged(index, index, QVector<int>() << role);
      return true;
    }
    return false;
  }*/

 /* Qt::ItemFlags PlaylistsDataModel::flags(const QModelIndex &index) const
  {
    if (!index.isValid()) {
    //  return Qt::NoItemFlags;
    }

    return Qt::ItemIsEditable; // FIXME: Implement me!
  }*/
/*
  bool PlaylistsDataModel::insertRows(int row, int count, const QModelIndex &parent) {
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();
  }

  bool PlaylistsDataModel::removeRows(int row, int count, const QModelIndex &parent) {
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
  }*/
}
