#include "directorydatamodel.h"

namespace Directory {
  DirectoryDataModel::DirectoryDataModel(const QString &library_path, QObject *parent) : QFileSystemModel(parent) {
    setReadOnly(true);
    setRootPath(library_path);
  }
}
