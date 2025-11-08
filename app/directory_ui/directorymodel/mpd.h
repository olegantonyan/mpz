#ifndef DIRECTORYDATAMODEMPD_H
#define DIRECTORYDATAMODEMPD_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

#include "mpd/client.h"

namespace DirectoryUi {
  namespace DirectoryModel {

    struct TreeItem {
      QString name;
      QString path;
      bool isDirectory;
      TreeItem* parent;
      QVector<TreeItem*> children;
      bool loaded;

      TreeItem(const QString& n, const QString& p, bool isDir, TreeItem* par = nullptr) : name(n), path(p), isDirectory(isDir), parent(par), loaded(false) {}

      ~TreeItem() {
        qDeleteAll(children);
      }
    };

    class Mpd: public QAbstractItemModel {
      Q_OBJECT

    public:
      explicit Mpd(QObject *parent = nullptr);
      ~Mpd();

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

    private:
      void load_directory(TreeItem* parent, const QString& path);
      QModelIndex createIndexForItem(TreeItem* item) const;

      struct mpd_connection *connection;
      TreeItem *root_item;

    };
  }
}

#endif // DIRECTORYDATAMODEMPD_H
