#include "wrapper.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
  Wrapper::Wrapper(QObject *parent) : QObject(parent), localfs(nullptr) {
      localfs = new Localfs(parent);
      connect(localfs, &Localfs::directoryLoaded, this, &DirectoryModel::Wrapper::directoryLoaded);
    }

    void Wrapper::loadAsync(const QString &path) {
      localfs->loadAsync(path);
    }

    QAbstractItemModel *Wrapper::model() const {
      return localfs;
    }

    QModelIndex Wrapper::rootIndex() const {
      return localfs->index(localfs->rootPath());
    }

    QString Wrapper::filePath(const QModelIndex &index) const {
      return localfs->filePath(index);
    }

    void Wrapper::setNameFilters(const QStringList &filters) {
      setNameFilters(filters);
    }

    void Wrapper::sortBy(const QString &direction) {
      localfs->sortBy(direction);
    }
  }
}
