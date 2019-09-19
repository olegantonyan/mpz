#include "playlistmodel.h"

namespace PlaylistUi {
  Model::Model(QObject *parent) : QAbstractTableModel(parent) {
  }

  int Model::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
      return 0;
    }

    // FIXME: Implement me!
    return 2;
  }

  int Model::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
      return 0;
    }

    // FIXME: Implement me!
    return 5;
  }

  QVariant Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    if (role == Qt::DisplayRole) {
      return QString("Row%1, Column%2").arg(index.row() + 1).arg(index.column() +1);
    }
    return QVariant();
  }
}
