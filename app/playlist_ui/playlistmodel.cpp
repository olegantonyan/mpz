#include "playlistmodel.h"

#include <QDebug>
#include <QFont>

namespace PlaylistUi {
  Model::Model(QObject *parent) : QAbstractTableModel(parent), highlight_uid(0) {
    tracks.clear();
  }

  QModelIndex Model::buildIndex(int row) const {
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

    if (role == Qt::TextAlignmentRole) {
      if (index.column() == 4 || index.column() == 5) {
        return (Qt::AlignRight + Qt::AlignVCenter);
      } else {
        return Qt::AlignVCenter;
      }
    }

    Track t = tracks.at(index.row());

    if (role == Qt::FontRole && t.uid() == highlight_uid) {
      QFont font;
      font.setBold(true);
      return font;
    }

    if (role == Qt::DisplayRole) {
      switch (index.column()) {
        case 0:
          if (highlight_uid == t.uid()) {
            if (highlight_state == Playing) {
              return "▶";
            } else if (highlight_state == Paused) {
              return "⏸";
            } else {
              return "";
            }
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
          if (t.year() == 0) {
            return QVariant();
          } else {
            return t.year();
          }
        case 5:
          return t.formattedDuration();
        default:
          throw "unhandled column number";
      }
    }
    return QVariant();
  }

  void Model::setTracks(const QVector<Track> &t) {
    beginRemoveRows(QModelIndex(), 0, tracks.size());
    tracks.clear();
    endRemoveRows();

    beginInsertRows(QModelIndex(), 0, t.size());
    tracks = t;
    endInsertRows();

    /*QModelIndex top = createIndex(0, 0);
    QModelIndex bottom = createIndex(tracks.size(), columnCount());
    emit dataChanged(top, bottom, {Qt::DisplayRole});*/
  }

  void Model::setPlaylist(std::shared_ptr<Playlist::Playlist> pl) {
    _playlist = pl;
    reload();
  }

  Track Model::itemAt(const QModelIndex &index) const {
    if (index.row() > tracks.size() - 1) {
      throw "index out of bounds";
    }
    return tracks.at(index.row());
  }

  int Model::tracksSize() const {
    return tracks.size();
  }

  void Model::highlight(quint64 uid, HighlightState st) {
    highlight_uid = uid;
    highlight_state = st;
    if (tracks.size() > 0) {
      emit dataChanged(buildIndex(0), buildIndex(tracks.size() - 1));
    }
  }

  QModelIndex Model::indexOf(quint64 uid) const {
    for (int i = 0; i < tracksSize(); i++) {
      if (uid == tracks.at(i).uid()) {
        return buildIndex(i);
      }
    }
    return buildIndex(-1);
  }

  std::shared_ptr<Playlist::Playlist> Model::playlist() {
    return _playlist;
  }

  void Model::reload() {
    if (_playlist == nullptr) {
      setTracks(QVector<Track>());
    } else {
      setTracks(_playlist->tracks());
    }
  }

  void Model::remove(const QList<QModelIndex> &items) {
    QList<QModelIndex> sorted = items;

    std::sort(sorted.begin(), sorted.end(), [](const QModelIndex &t1, const QModelIndex &t2) -> bool {
      return t1.row() > t2.row();
    });
    for (auto i : sorted) {
      playlist()->removeTrack(i.row());
    }
    reload();
  }
}
