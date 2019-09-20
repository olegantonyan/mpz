#include "playlistmodel.h"

#include <QDebug>

namespace PlaylistUi {
  Model::Model(QObject *parent) : QAbstractTableModel(parent) {
    tracks.clear();
  }

  int Model::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
      return 0;
    }
    return tracks.size();
  }

  int Model::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
      return 0;
    }
    return 5;
  }

  QVariant Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    if (role == Qt::DisplayRole) {
      Track t = tracks.at(index.row());
      switch (index.column()) {
        case 0:
          return t.artist();
        case 1:
          return t.album();
        case 2:
          return t.title();
        case 3:
          return t.year();
        case 4:
          return "";
        default:
          throw "unhandled column number";
      }
    }
    return QVariant();
  }

  void Model::setTracks(const QVector<Track> &t) {
    beginInsertRows(QModelIndex(), 0, columnCount());
    tracks = t;
    endInsertRows();
    QModelIndex top = createIndex(0, 0);
    QModelIndex bottom = createIndex(tracks.size(), columnCount());
    emit dataChanged(top, bottom, {Qt::DisplayRole});
  }
}
