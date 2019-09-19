#ifndef DIRECTORYDATAMODEL_H
#define DIRECTORYDATAMODEL_H

#include <QFileSystemModel>
#include <QString>

namespace DirectoryUi {
  class Model : public QFileSystemModel {
    Q_OBJECT

  public:
    explicit Model(const QString &library_path, QObject *parent = nullptr);
  };
}

#endif // DIRECTORYDATAMODEL_H
