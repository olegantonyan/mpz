#ifndef DIRECTORYDATAMODELWRAPPER_H
#define DIRECTORYDATAMODELWRAPPER_H

#include "localfs.h"

#include <QObject>
#include <QString>
#include <QAbstractItemModel>

namespace DirectoryUi {
  namespace DirectoryModel {
    class Wrapper : public QObject {
      Q_OBJECT

    public:
      explicit Wrapper(QObject *parent = nullptr);

      void loadAsync(const QString &path);

      QAbstractItemModel *model() const;
      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;
      void setNameFilters(const QStringList &filters);

    signals:
      void directoryLoaded(const QString &path);

    public slots:
      void sortBy(const QString &direction);

    private:
      Localfs *localfs;
    };
  }
}

#endif // DIRECTORYDATAMODELWRAPPER_H
