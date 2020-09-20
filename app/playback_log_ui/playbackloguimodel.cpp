#include "playbackloguimodel.h"

namespace PlaybackLogUi {
  Model::Model(int max_sz, QObject *parent) : QAbstractTableModel(parent), max_size(max_sz) {
  }

  int Model::rowCount(const QModelIndex &parent) const{
    if (parent.isValid()) {
      return 0;
    }
    return items.size();
  }

  int Model::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
      return 0;
    }
    return 2;
  }

  QVariant Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    auto item = items.at(index.row());

    if (role == Qt::DisplayRole) {
      switch (index.column()) {
        case 0:
          return item.time.toString("HH:mm:ss");
        case 1:
          return item.text;
        default:
          break;
      }
    }
    return QVariant();
  }

  Item Model::last() const {
    return items.first();
  }

  Item Model::itemAt(const QModelIndex &index) const {
    return items.at(index.row());
  }

  void Model::append(const Item &item) {
    if (items.size() > max_size) {
      beginRemoveRows(QModelIndex(), 0, 0);
      items.pop_back();
      endRemoveRows();
    }
    beginInsertRows(QModelIndex(), items.size(), items.size());
    items.push_front(item);
    endInsertRows();

    emit changed();
  }
}