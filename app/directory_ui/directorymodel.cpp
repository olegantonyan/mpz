#include "directorymodel.h"

#include <QDebug>

namespace DirectoryUi {
  Model::Model(const QString &library_path, QObject *parent) : QFileSystemModel(parent) {
    setReadOnly(true);
    setRootPath(library_path);
  }
}
