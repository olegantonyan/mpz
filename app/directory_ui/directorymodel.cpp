#include "directorymodel.h"

#include <QDebug>

namespace DirectoryUi {
  Model::Model(QObject *parent) : QFileSystemModel(parent) {
    setReadOnly(true);
  }

  void Model::loadAsync(const QString &path) {
    setRootPath(path);
    //sort(3, Qt::DescendingOrder);
  }
}
