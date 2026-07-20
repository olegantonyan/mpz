#include "playlistsmodel.h"
#include "playlist/loader.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFont>
#include <QMimeData>
#include <QDataStream>
#include <QTimer>

namespace {
  const QString playlistRowMime = QStringLiteral("application/x-mpz-playlist-row");
}

namespace PlaylistsUi {
  Model::Model(Config::Local &conf, QObject *parent) : QAbstractListModel(parent), local_conf(conf) {
    list.clear();
  }

  void Model::loadAsync() {
    QFuture<void> future = QtConcurrent::run(QThreadPool::globalInstance(), [this]() {
      auto newList = local_conf.playlists();

      auto lambda = [this, newList = std::move(newList)]() mutable {
        beginResetModel();
        list = std::move(newList);
        endResetModel();
        emit asyncLoadFinished();
        persist();
      };
      QTimer::singleShot(0, this, lambda);
    });
    Q_UNUSED(future);
  }

  void Model::asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist) {
    emit asyncTracksLoadFinished(playlist);
  }

  void Model::appendTracksToPlaylist(std::shared_ptr<Playlist::Playlist> playlist, const QVector<Track> &tracks) {
    if (!playlist || tracks.isEmpty()) {
      return;
    }
    playlist->append(tracks, false);
    emit asyncTracksLoadFinished(playlist);
  }

  void Model::higlight(std::shared_ptr<Playlist::Playlist> playlist) {
    if (playlist == nullptr) {
      highlight_uid = 0;
      if (!list.isEmpty()) {
        emit dataChanged(buildIndex(0), buildIndex(list.size() - 1));
      }
    } else {
      highlight_uid = playlist->uid();
      emit dataChanged(itemIndex(playlist), itemIndex(playlist));
    }
  }

  QModelIndex Model::buildIndex(int row) const {
    return createIndex(row, 0);
  }

  QModelIndex Model::append(std::shared_ptr<Playlist::Playlist> item) {
    beginInsertRows(QModelIndex(), list.size(), list.size());
    list.append(item);
    endInsertRows();
    QModelIndex idx = buildIndex(list.size() - 1);
    emit dataChanged(idx, idx);
    persist();
    return idx;
  }

  void Model::remove(const QModelIndex &index) {
    if (index.row() > list.size() - 1) {
      return;
    }
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    list.removeAt(index.row());
    endRemoveRows();
    persist();
  }

  std::shared_ptr<Playlist::Playlist> Model::itemAt(const QModelIndex &index) const {
    if (!index.isValid()) {
      return nullptr;
    }
    if (index.row() > list.size() - 1) {
      return nullptr;
    }
    return list.at(index.row());
  }

  std::shared_ptr<Playlist::Playlist> Model::itemBy(quint64 uid) const {
    for (const auto &i : std::as_const(list)) {
      if (i->uid() == uid) {
        return i;
      }
    }
    return nullptr;
  }

  std::shared_ptr<Playlist::Playlist> Model::itemByTrack(quint64 track_uid) const {
    for (const auto &i : std::as_const(list)) {
      if (i->hasTrack(track_uid)) {
        return i;
      }
    }
    return nullptr;
  }

  int Model::listSize() const {
    return list.size();
  }

  QModelIndex Model::itemIndex(std::shared_ptr<Playlist::Playlist> playlist) const {
    for (int i = 0; i < list.size(); i++) {
      if (playlist->uid() == list.at(i)->uid()) {
        return buildIndex(i);
      }
    }
    return buildIndex(-1);
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
    QVariant none;

    if (!index.isValid() || index.row() >= list.size()) {
      return none;
    }

    auto pl = list.at(index.row());
    if (!pl) {
      return none;
    }
    if (role == Qt::FontRole && pl->uid() == highlight_uid) {
      QFont font;
      font.setBold(true);
      return font;
    }

    if (role == Qt::DisplayRole) {
      if (list.size() > index.row()) {
        return list.at(index.row())->name();
      } else {
        return none;
      }
    }

    return none;
  }

  bool Model::persist() {
    return local_conf.savePlaylists(list);
  }

  Qt::ItemFlags Model::flags(const QModelIndex &index) const {
    auto defaults = QAbstractListModel::flags(index);
    if (index.isValid()) {
      return defaults | Qt::ItemIsDragEnabled;
    }
    return defaults | Qt::ItemIsDropEnabled;
  }

  Qt::DropActions Model::supportedDropActions() const {
    return Qt::MoveAction;
  }

  QStringList Model::mimeTypes() const {
    return {playlistRowMime};
  }

  QMimeData *Model::mimeData(const QModelIndexList &indexes) const {
    if (indexes.isEmpty() || !indexes.first().isValid()) {
      return nullptr;
    }
    auto *data = new QMimeData;
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << indexes.first().row();
    data->setData(playlistRowMime, bytes);
    return data;
  }

  bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    Q_UNUSED(column)
    if (action != Qt::MoveAction || !data || !data->hasFormat(playlistRowMime)) {
      return false;
    }

    int src = -1;
    QByteArray bytes = data->data(playlistRowMime);
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    stream >> src;
    if (src < 0 || src >= list.size()) {
      return false;
    }

    int dst = row;
    if (dst < 0) {
      dst = parent.isValid() ? parent.row() : list.size();
    }
    if (dst < 0 || dst > list.size()) {
      return false;
    }
    if (dst == src || dst == src + 1) {
      return false;
    }

    beginMoveRows(QModelIndex(), src, src, QModelIndex(), dst);
    list.move(src, dst > src ? dst - 1 : dst);
    endMoveRows();
    persist();
    return false;
  }

  QList<std::shared_ptr<Playlist::Playlist> > Model::itemList() const {
    return list;
  }

  QModelIndex Model::currentPlaylistIndex() {
    return buildIndex(qMin(local_conf.currentPlaylist(), listSize() - 1));
  }

  void Model::saveCurrentPlaylistIndex(const QModelIndex &idx) {
    int current_index = idx.row();
    auto max_index = qMax(listSize() - 1, 0);
    auto save_index = qMin(current_index, max_index);
    local_conf.saveCurrentPlaylist(save_index);
  }

  void Model::createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) {
    Q_ASSERT(filepaths.size() > 0);

    auto pl = std::shared_ptr<Playlist::Playlist>(new Playlist::Playlist());

    (void)QtConcurrent::run(QThreadPool::globalInstance(), [this, filepaths, libraryDir, pl]() -> void {
      QStringList names;
      for (const auto &path : std::as_const(filepaths)) {
        Playlist::Loader loader(path);
        pl->append(loader.tracks(), !loader.is_playlist_file());
        names << playlistNameBy(path, libraryDir);
      }
      pl->rename(names.join(", "));
      emit createPlaylistAsyncFinished(pl);
    });
  }

  void Model::createPlaylistFromTracks(const QVector<Track> &tracks, const QString &name) {
    if (tracks.isEmpty()) {
      return;
    }

    auto pl = std::shared_ptr<Playlist::Playlist>(new Playlist::Playlist());
    pl->append(tracks, false);
    pl->rename(name);

    // createPlaylistAsync emits from a worker thread, so the finished handler
    // always runs queued. Defer here too, or it would re-enter the view that is
    // currently dispatching the double-click.
    QTimer::singleShot(0, this, [this, pl]() {
      emit createPlaylistAsyncFinished(pl);
    });
  }

  QString Model::playlistNameBy(const QDir &path, const QString &libraryDir) {
    auto result = path.absolutePath().remove(libraryDir);
    if (result.startsWith("/")) {
      return result.remove(0, 1);
    } else {
      return result;
    }
  }
}
