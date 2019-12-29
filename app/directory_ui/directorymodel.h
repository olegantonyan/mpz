#ifndef DIRECTORYDATAMODEL_H
#define DIRECTORYDATAMODEL_H

#include <QFileSystemModel>
#include <QString>

namespace DirectoryUi {
  class Model : public QFileSystemModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    void loadAsync(const QString &path);
  };
}

#endif // DIRECTORYDATAMODEL_H
