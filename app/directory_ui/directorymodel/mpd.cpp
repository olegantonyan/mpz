#include "mpd.h"

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpd::Mpd(QObject *parent) : QAbstractItemModel{parent} {
    }

    void Mpd::loadAsync(const QString &path) {
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::index(int row, int column, const QModelIndex &parent) const {
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::parent(const QModelIndex &child) const {
    }

    int DirectoryUi::DirectoryModel::Mpd::rowCount(const QModelIndex &parent) const {
    }

    int DirectoryUi::DirectoryModel::Mpd::columnCount(const QModelIndex &parent) const {
    }

    QVariant DirectoryUi::DirectoryModel::Mpd::data(const QModelIndex &index, int role) const {
    }
  }
}

