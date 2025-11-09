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
      QModelIndex rootIndex() const;

      // QAbstractItemModel interface
      int columnCount(const QModelIndex &parent) const;
    };
  }
}

#endif // DIRECTORYDATAMODELLOCAL_H
