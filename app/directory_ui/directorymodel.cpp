#include "directorymodel.h"

namespace DirectoryUi {
  Model::Model(const QString &library_path, QObject *parent) : QFileSystemModel(parent) {
    setReadOnly(true);
    setRootPath(library_path);
  }
}
