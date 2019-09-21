#include "playlistsmodel.h"

#include <QDebug>

namespace PlaylistsUi {
  Model::Model(QObject *parent) : QAbstractListModel(parent) {
    list.clear();
  }

  QModelIndex Model::append(std::shared_ptr<Playlist> item) {
    QModelIndex idx = createIndex(list.size(), 0);
    list.append(item);
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

  std::shared_ptr<Playlist> Model::itemAt(const QModelIndex &index) const {
    if (index.row() > list.size() - 1) {
      throw "index out of bounds";
    }
    return list.at(index.row());
  }

  int Model::listSize() const {
    return list.size();
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
        return list.at(index.row())->name();
      } else {
        return QVariant();
      }
    }

    return QVariant();
  }
}
