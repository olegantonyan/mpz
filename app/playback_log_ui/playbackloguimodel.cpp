#include "playbackloguimodel.h"

#include <QStringList>

namespace PlaybackLogUi {
  Model::Model(Config::Local &local_c, int max_sz, QObject *parent) : QAbstractTableModel(parent), local_config(local_c), max_size(max_sz) {
    total_play_time = local_config.totalPlaybackTime();
    this_session_play_time = 0;
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

  QString Model::itemsToCsv() const {
    QStringList result;
    for (int i = 0; i < rowCount(); i++) {
      QStringList row;
      for (int j = 0; j < columnCount(); j++) {
        row += data(createIndex(i, j)).toString();
      }
      result += row.join(",");
    }
    return result.join("\n");
  }

  void Model::append(const Item &item) {
    if (items.size() >= max_size) {
      beginRemoveRows(QModelIndex(), 0, 0);
      items.pop_back();
      endRemoveRows();
    }
    beginInsertRows(QModelIndex(), items.size(), items.size());
    items.push_front(item);
    endInsertRows();

    emit changed();
  }

  void Model::incrementPlayTime(int by) {
    total_play_time += by;
    this_session_play_time += by;
    emit totalPlayTimeChanged(total_play_time);
    emit thisSessionPlayTimeChanged(this_session_play_time);
    local_config.saveTotalPlaybackTime(total_play_time);
  }
}
