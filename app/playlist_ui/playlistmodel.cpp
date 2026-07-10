#include "playlistmodel.h"
#include "playlist/loader.h"
#include "icons.h"

#include <QDebug>
#include <QFont>
#include <QtConcurrent>
#include <QMimeData>
#include <QDataStream>
#include <QSet>

namespace {
  const QString playlistTracksMime = QStringLiteral("application/x-mpz-playlist-tracks");
}

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
    QVariant none;

    if (!index.isValid()) {
      return none;
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
             return Icons::get(Icons::Icon::Play);
           } else if (highlight_state == Paused) {
             return Icons::get(Icons::Icon::Pause);
           }
         }
       }
    }

    if (role == Qt::DisplayRole && index.column() > 0) {
      return columns_config.value(index.column(), t);
    }
    return none;
  }

  void Model::setTracks(const QVector<Track> &t) {
    if (!tracks.isEmpty()) {
      beginRemoveRows(QModelIndex(), 0, tracks.size() - 1);
      tracks.clear();
      endRemoveRows();
    }

    if (!t.isEmpty()) {
      beginInsertRows(QModelIndex(), 0, t.size() - 1);
      tracks = t;
      endInsertRows();
    }

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
    if (index.row() < 0 || index.row() >= tracks.size()) {
      return Track();
    }
    return tracks.at(index.row());
  }

  const Track &Model::trackAt(int row) const {
    return tracks.at(row);
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
    QList<int> indecies;
    for (const auto &i : std::as_const(sorted)) {
      indecies << i.row();
    }
    removeTracksFromPlaylist(indecies);
    reload();
  }

  void Model::removeTracksFromPlaylist(const QList<int> &indecies) {
    for (int i : indecies) {
      playlist()->removeTrack(i);
    }
  }

  void Model::appendToPlaylistAsync(const QList<QDir> &filepaths) {
    auto pl = playlist();
    if (!pl) {
      return;
    }

    (void)QtConcurrent::run(QThreadPool::globalInstance(), [this, filepaths, pl]() -> void {
      for (const auto &path : std::as_const(filepaths)) {
        Playlist::Loader loader(path);
        pl->append(loader.tracks(), !loader.is_playlist_file());
      }

      emit appendToPlaylistAsyncFinished(pl);
    });
  }

  void Model::insertTracksAsync(const QList<QDir> &filepaths, int atRow) {
    auto pl = playlist();
    if (!pl) {
      return;
    }

    (void)QtConcurrent::run(QThreadPool::globalInstance(), [this, filepaths, atRow, pl]() -> void {
      Playlist::Playlist batch;
      for (const auto &path : std::as_const(filepaths)) {
        Playlist::Loader loader(path);
        batch.append(loader.tracks(), !loader.is_playlist_file());
      }

      auto merged = pl->tracks();
      const int pos = qBound(0, atRow, static_cast<int>(merged.size()));
      const auto incoming = batch.tracks();
      for (int i = 0; i < incoming.size(); ++i) {
        merged.insert(pos + i, incoming.at(i));
      }
      pl->load(merged);

      emit appendToPlaylistAsyncFinished(pl);
    });
  }

  void Model::sortBy(const QString &criteria) {
    playlist()->sortBy(criteria);
    reload();
  }

  Qt::ItemFlags Model::flags(const QModelIndex &index) const {
    auto defaults = QAbstractTableModel::flags(index);
    if (index.isValid()) {
      return defaults | Qt::ItemIsDragEnabled;
    }
    return defaults | Qt::ItemIsDropEnabled;
  }

  Qt::DropActions Model::supportedDropActions() const {
    return Qt::MoveAction;
  }

  QStringList Model::mimeTypes() const {
    return {playlistTracksMime};
  }

  QMimeData *Model::mimeData(const QModelIndexList &indexes) const {
    QSet<int> unique;
    for (const auto &idx : indexes) {
      if (idx.isValid()) {
        unique.insert(idx.row());
      }
    }
    if (unique.isEmpty()) {
      return nullptr;
    }
    QList<int> rows = unique.values();
    std::sort(rows.begin(), rows.end());

    auto *data = new QMimeData;
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << static_cast<qint32>(rows.size());
    for (int r : rows) {
      stream << static_cast<qint32>(r);
    }
    data->setData(playlistTracksMime, bytes);
    return data;
  }

  bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    Q_UNUSED(column)
    if (action != Qt::MoveAction || !data || !data->hasFormat(playlistTracksMime) || !playlist()) {
      return false;
    }

    QByteArray bytes = data->data(playlistTracksMime);
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    qint32 count = 0;
    stream >> count;
    QList<int> srcRows;
    srcRows.reserve(count);
    for (qint32 i = 0; i < count; i++) {
      qint32 r = -1;
      stream >> r;
      if (r >= 0 && r < tracks.size()) {
        srcRows.append(r);
      }
    }
    if (srcRows.isEmpty()) {
      return false;
    }
    std::sort(srcRows.begin(), srcRows.end());

    int dst = row;
    if (dst < 0) {
      dst = parent.isValid() ? parent.row() : tracks.size();
    }
    dst = qBound(0, dst, tracks.size());

    int below = 0;
    for (int r : srcRows) {
      if (r < dst) below++;
    }
    int adjustedDst = dst - below;

    QVector<Track> moved;
    moved.reserve(srcRows.size());
    for (int r : srcRows) {
      moved.append(tracks.at(r));
    }

    auto applyMove = [this, &moved, adjustedDst](int i) {
      const quint64 uid = moved.at(i).uid();
      int cur = -1;
      for (int j = 0; j < tracks.size(); j++) {
        if (tracks.at(j).uid() == uid) {
          cur = j;
          break;
        }
      }
      if (cur < 0) {
        return;
      }
      int target = adjustedDst + i;
      if (cur == target) {
        return;
      }
      int qtTarget = target > cur ? target + 1 : target;
      beginMoveRows(QModelIndex(), cur, cur, QModelIndex(), qtTarget);
      tracks.move(cur, target);
      endMoveRows();
    };

    for (int i = below - 1; i >= 0; i--) {
      applyMove(i);
    }
    for (int i = below; i < moved.size(); i++) {
      applyMove(i);
    }

    playlist()->load(tracks);
    return false;
  }
}
