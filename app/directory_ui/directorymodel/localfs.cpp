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

    void Localfs::sortBy(const QString &direction) {
      if (direction.toLower() == "default") {
        sort(0, Qt::AscendingOrder);
      } else if (direction.toLower() == "date") {
        sort(3, Qt::AscendingOrder);
      } else if (direction.toLower() == "- date") {
        sort(3, Qt::DescendingOrder);
      }
    }
  }
}
