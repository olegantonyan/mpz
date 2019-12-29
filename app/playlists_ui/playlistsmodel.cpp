#include "playlistsmodel.h"

#include <QDebug>
#include <QtConcurrent>

namespace PlaylistsUi {  
  Model::Model(Config::Local &conf, QObject *parent) : QAbstractListModel(parent), local_conf(conf) {
    list.clear();
  }

  void Model::loadAsync() {
    QtConcurrent::run([=]() {
      emit asynLoadStarted();
      list = local_conf.playlists();
      emit dataChanged(buildIndex(0), buildIndex(list.size()), {Qt::DisplayRole});
      emit asynLoadFinished();
      persist();
    });
  }

  QModelIndex Model::buildIndex(int row) const {
    return createIndex(row, 0);
  }

  QModelIndex Model::append(std::shared_ptr<Playlist> item) {
    QModelIndex idx = buildIndex(list.size());
    list.append(item);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
    persist();
    return idx;
  }

  void Model::remove(const QModelIndex &index) {
    if (index.row() > list.size() - 1) {
      return;
    }
    list.removeAt(index.row());
    persist();
    emit dataChanged(index, index, {Qt::DisplayRole});
  }

  std::shared_ptr<Playlist> Model::itemAt(const QModelIndex &index) const {
    if (!index.isValid()) {
      return nullptr;
    }
    if (index.row() > list.size() - 1) {
      return nullptr;
    }
    return list.at(index.row());
  }

  std::shared_ptr<Playlist> Model::itemBy(quint64 uid) const {
    for (auto i : list) {
      if (i->uid() == uid) {
        return i;
      }
    }
    return nullptr;
  }

  std::shared_ptr<Playlist> Model::itemByTrack(quint64 track_uid) const {
    for (auto i : list) {
      if (i->hasTrack(track_uid)) {
        return i;
      }
    }
    return nullptr;
  }

  int Model::listSize() const {
    return list.size();
  }

  QModelIndex Model::itemIndex(std::shared_ptr<Playlist> playlist) const {
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

  bool Model::persist() {
    return local_conf.savePlaylists(list);
  }

  QList<std::shared_ptr<Playlist> > Model::itemList() const {
    return list;
  }
}
