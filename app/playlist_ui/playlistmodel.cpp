#include "playlistmodel.h"

#include <QDebug>
#include <QFont>

namespace PlaylistUi {
  Model::Model(QStyle *stl, const ColumnsConfig &col_cfg, QObject *parent) : QAbstractTableModel(parent), highlight_uid(0), style(stl), columns_config(col_cfg) {
    tracks.clear();
    _playlist = nullptr;
  }

  QModelIndex Model::buildIndex(int row, int col) const {
    return createIndex(row, col);
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
    return columns_config.count() + 1;
  }

  QVariant Model::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
      return QVariant();
    }

    if (role == Qt::TextAlignmentRole) {
      if (index.column() == 0) {
        return Qt::AlignVCenter;
      } else {
        return QVariant(columns_config.align(index.column()));
      }
    }

    Track t = tracks.at(index.row());

    if (role == Qt::FontRole) {
      QFont font;
      font.setBold(t.uid() == highlight_uid);
      return font;
    }

    if (role == Qt::DecorationRole) {
       if (index.column() == 0) {
         if (highlight_uid == t.uid()) {
           if (highlight_state == Playing) {
             return style->standardIcon(QStyle::SP_MediaPlay);
           } else if (highlight_state == Paused) {
             return style->standardIcon(QStyle::SP_MediaPause);
           }
         }
       }
    }

    if (role == Qt::DisplayRole && index.column() > 0) {
      return columns_config.value(index.column(), t);
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

  void Model::allDataChanged() {
    for (int col = 0; col < columnCount(); col++) {
      emit dataChanged(buildIndex(0, col), buildIndex(tracks.size() - 1, col));
    }
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
       allDataChanged();
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
