#include "playlistmodel.h"

#include <QDebug>
#include <QFont>

namespace PlaylistUi {
  Model::Model(QObject *parent) : QAbstractTableModel(parent), highlight_row(-1) {
    tracks.clear();
  }

  QModelIndex Model::buildIndex(int row) {
    return createIndex(row, 0);
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
    return 6;
  }

  QVariant Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    if ((index.column() == 4 || index.column() == 5) && role == Qt::TextAlignmentRole) {
      return Qt::AlignRight;
    }

    if (role == Qt::FontRole && highlight_row >= 0 && index.row() == highlight_row && highlight_playlist == current_playlist_index()) {
      QFont font;
      font.setBold(true);
      return font;
    }

    if (role == Qt::DisplayRole) {
      Track t = tracks.at(index.row());
      switch (index.column()) {
        case 0:
          if (highlight_row >= 0 && highlight_row == index.row() && highlight_playlist == current_playlist_index()) {
            //return "►";
            return "▷";
          } else {
            return "";
          }
        case 1:
          return t.artist();
        case 2:
          return t.album();
        case 3:
          return t.title();
        case 4:
          return t.year();
        case 5:
          return t.formattedDuration();
        default:
          throw "unhandled column number";
      }
    }
    return QVariant();
  }

  void Model::setTracks(const QVector<Track> &t, int index) {
    beginRemoveRows(QModelIndex(), 0, tracks.size());
    tracks.clear();
    endRemoveRows();

    beginInsertRows(QModelIndex(), 0, t.size());
    tracks = t;
    endInsertRows();

    playlist_index = index;
    /*QModelIndex top = createIndex(0, 0);
    QModelIndex bottom = createIndex(tracks.size(), columnCount());
    emit dataChanged(top, bottom, {Qt::DisplayRole});*/
  }

  Track Model::itemAt(const QModelIndex &index) const {
    if (index.row() > tracks.size() - 1) {
      throw "index out of bounds";
    }
    return tracks.at(index.row());
  }

  int Model::current_playlist_index() const {
    return playlist_index;
  }

  int Model::tracksSize() const {
    return tracks.size();
  }

  void Model::highlight(int row, int playlist) {
    highlight_row = row;
    highlight_playlist = playlist;
    emit dataChanged(buildIndex(row), buildIndex(row));
  }
}
