#include "localfs.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
    Localfs::Localfs(QObject *parent) : QFileSystemModel(parent) {
      setReadOnly(true);
      setNameFilterDisables(false);
      setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    }

    void Localfs::loadAsync(const QString &path) {
      if (rootPath() != path) {
        setRootPath(path);
      } else {
        emit directoryLoaded(path); // emit anyway to let view refresh
      }
    }

    QModelIndex Localfs::rootIndex() const {
      return index(rootPath());
    }

    int Localfs::columnCount(const QModelIndex &parent) const {
      Q_UNUSED(parent);
      return 1;
    }
  }
}



