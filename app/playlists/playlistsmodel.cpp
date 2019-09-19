#include "playlistsmodel.h"

#include <QDebug>

namespace Playlists {
  Model::Model(QObject *parent) : QAbstractListModel(parent) {
  }

  QModelIndex Model::append(const PlaylistItem &item) {
    list.append(item);
    QModelIndex idx = createIndex(list.size() - 1, 0);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
    return idx;
  }

  void Model::remove(const QModelIndex &index) {
    if (index.row() > list.size() - 1) {
      return;
    }
    list.removeAt(index.row());
    emit dataChanged(index, index, {Qt::DisplayRole});
  }

  PlaylistItem Model::itemAt(const QModelIndex &index) const {
    if (index.row() > list.size() - 1) {
      throw "index out of bounds";
    }
    return list.at(index.row());
  }

  PlaylistItem Model::repalceAt(const QModelIndex &index, const PlaylistItem &newItem) {
    if (index.row() > list.size() - 1) {
      throw "index out of bounds";
    }
    list.replace(index.row(), newItem);
    return itemAt(index);
  }

  int Model::rowCount(const QModelIndex &parent) const {
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid()) {
      return 0;
    }

    return list.size();
  }

  QVariant Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    if (role == Qt::DisplayRole) {
      if (list.size() > index.row()) {
        return list.at(index.row()).getName();
      } else {
        return QVariant();
      }
    }

    return QVariant();
  }
}
