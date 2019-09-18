#ifndef DIRECTORYDATAMODEL_H
#define DIRECTORYDATAMODEL_H

#include <QFileSystemModel>
#include <QString>

namespace Directory {
  class DirectoryDataModel : public QFileSystemModel {
    Q_OBJECT

  public:
    explicit DirectoryDataModel(const QString &library_path, QObject *parent = nullptr);
  };
}

#endif // DIRECTORYDATAMODEL_H
