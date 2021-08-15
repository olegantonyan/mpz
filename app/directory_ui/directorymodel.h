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

  public slots:
    void sortBy(const QString &direction);
  };
}

#endif // DIRECTORYDATAMODEL_H
