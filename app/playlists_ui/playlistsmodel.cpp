#include "playlistsmodel.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFont>

namespace PlaylistsUi {  
  Model::Model(Config::Local &conf, QObject *parent) : QAbstractListModel(parent), local_conf(conf) {
    list.clear();
  }

  void Model::loadAsync() {
    (void)QtConcurrent::run([=]() {
      list = local_conf.playlists();
      beginInsertRows(QModelIndex(), 0, list.size());
      endInsertRows();
      emit dataChanged(buildIndex(0), buildIndex(list.size()));
      emit asynLoadFinished();
      persist();
    });
  }

  void Model::higlight(std::shared_ptr<Playlist::Playlist> playlist) {
    if (playlist == nullptr) {
      highlight_uid = 0;
      emit dataChanged(buildIndex(0), buildIndex(list.size() - 1));
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
    QModelIndex idx = buildIndex(list.size());
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
    emit dataChanged(index, index);
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
    for (auto i : list) {
      if (i->uid() == uid) {
        return i;
      }
    }
    return nullptr;
  }

  std::shared_ptr<Playlist::Playlist> Model::itemByTrack(quint64 track_uid) const {
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
    if (!index.isValid()) {
      return QVariant();
    }

    auto pl = list.at(index.row());
    if (role == Qt::FontRole && pl->uid() == highlight_uid) {
      QFont font;
      font.setBold(true);
      return font;
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

  QList<std::shared_ptr<Playlist::Playlist> > Model::itemList() const {
    return list;
  }
}
