#include "mpd.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpd::Mpd(QObject *parent) : QAbstractItemModel{parent} {
    }

    void Mpd::loadAsync(const QString &path) {
      qDebug() << "MPD load async" << path;
    }

    void Mpd::setNameFilters(const QStringList &filters) {
    }

    QModelIndex Mpd::rootIndex() const {
      return QModelIndex();
    }

    QString Mpd::filePath(const QModelIndex &index) const {
      return "";
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::index(int row, int column, const QModelIndex &parent) const {
      return QModelIndex();
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::parent(const QModelIndex &child) const {
      return QModelIndex();
    }

    int DirectoryUi::DirectoryModel::Mpd::rowCount(const QModelIndex &parent) const {
      return 0;
    }

    int DirectoryUi::DirectoryModel::Mpd::columnCount(const QModelIndex &parent) const {
      return 0;
    }

    QVariant DirectoryUi::DirectoryModel::Mpd::data(const QModelIndex &index, int role) const {
      return QVariant("0");
    }
  }
}

