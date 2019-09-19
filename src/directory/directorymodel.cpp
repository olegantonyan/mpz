#include "directorymodel.h"

namespace Directory {
  Model::Model(const QString &library_path, QObject *parent) : QFileSystemModel(parent) {
    setReadOnly(true);
    setRootPath(library_path);
  }
}
