#ifndef DIRECTORYDATAMODELLOCAL_H
#define DIRECTORYDATAMODELLOCAL_H

#include <QFileSystemModel>
#include <QString>

namespace DirectoryUi {
  namespace DirectoryModel {
    class Localfs : public QFileSystemModel {
      Q_OBJECT

    public:
      explicit Localfs(QObject *parent = nullptr);

      void loadAsync(const QString &path);

    public slots:
      void sortBy(const QString &direction);
    };
  }
}

#endif // DIRECTORYDATAMODELLOCAL_H
