#include "directorymodel.h"

#include <QDebug>

namespace DirectoryUi {
  Model::Model(QObject *parent) : QFileSystemModel(parent) {
    setReadOnly(true);
    setNameFilterDisables(false);
    setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
  }

  void Model::loadAsync(const QString &path) {
    setRootPath(path);
  }

  void Model::sortBy(const QString &direction) {
    if (direction.toLower() == "default") {
      sort(0, Qt::AscendingOrder);
    } else if (direction.toLower() == "date") {
      sort(3, Qt::AscendingOrder);
    } else if (direction.toLower() == "- date") {
      sort(3, Qt::DescendingOrder);
    }
  }
}
