#ifndef DIRECTORYDATAMODEMPD_H
#define DIRECTORYDATAMODEMPD_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QStringList>

#include "mpdconnection.h"
#include "mpd/treeitem.h"

namespace DirectoryUi {
  namespace DirectoryModel {
    class Mpd: public QAbstractItemModel {
      Q_OBJECT

    signals:
      void directoryLoaded(const QString &path);

    public:
      explicit Mpd(MpdConnection &conn, QObject *parent = nullptr);
      ~Mpd();

      void loadAsync(const QString &path);
      void filter(const QString &term);
      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;

      // QAbstractItemModel interface
      QModelIndex index(int row, int column, const QModelIndex &parent) const;
      QModelIndex parent(const QModelIndex &child) const;
      int rowCount(const QModelIndex &parent) const;
      int columnCount(const QModelIndex &parent) const;
      QVariant data(const QModelIndex &index, int role) const;
      bool canFetchMore(const QModelIndex &parent) const;
      void fetchMore(const QModelIndex &parent);
      bool hasChildren(const QModelIndex &parent) const;
      void sort(int column, Qt::SortOrder order);

    public slots:
      void on_database_updated();

    private:
      void load_directory(TreeItem* parent, const QString& path);
      QModelIndex create_index_for_item(TreeItem* item) const;
      TreeItem *tree_item_from_index(const QModelIndex &index) const;
      TreeItem *create_root_item();

      MpdConnection &connection;
      TreeItem *root_item;
    };
  }
}

#endif // DIRECTORYDATAMODEMPD_H
