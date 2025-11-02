#ifndef DIRECTORYDATAMODEMPD_H
#define DIRECTORYDATAMODEMPD_H

#include <QAbstractItemModel>
#include <QObject>

#include "mpd/client.h"

namespace DirectoryUi {
  namespace DirectoryModel {
    class Mpd: public QAbstractItemModel {
      Q_OBJECT

    public:
      explicit Mpd(QObject *parent = nullptr);

      void loadAsync(const QString &path);
      void setNameFilters(const QStringList &filters);
      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;

      // QAbstractItemModel interface
      QModelIndex index(int row, int column, const QModelIndex &parent) const;
      QModelIndex parent(const QModelIndex &child) const;
      int rowCount(const QModelIndex &parent) const;
      int columnCount(const QModelIndex &parent) const;
      QVariant data(const QModelIndex &index, int role) const;
    };
  }
}

#endif // DIRECTORYDATAMODEMPD_H
