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
      setRootPath(path);
    }
  }
}
