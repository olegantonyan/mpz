#ifndef DIRECTORYDATAMODEMPD_H
#define DIRECTORYDATAMODEMPD_H

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QStringList>
#if (QT_VERSION <= QT_VERSION_CHECK(5, 14, 0))
  #include <QMutex>
#else
  #include <QRecursiveMutex>
#endif

#include "mpd_client/client.h"
#include "mpd/treeitem.h"

namespace DirectoryUi {
  namespace DirectoryModel {
    class Mpd: public QAbstractItemModel {
      Q_OBJECT

    signals:
      void directoryLoaded(const QString &path);

    public:
      explicit Mpd(MpdClient::Client &cl, QObject *parent = nullptr);
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
      void onDatabaseUpdated();
      void onMpdReady();
      void onMpdLost();

    private:
      void loadDirectory(TreeItem* parent, const QString& path);
      QModelIndex createIndexForItem(TreeItem* item) const;
      TreeItem *treeItemFromIndex(const QModelIndex &index) const;
      TreeItem *createRootItem();

      MpdClient::Client &client;
      TreeItem *root_item;

      int last_sort_column;
      Qt::SortOrder last_sort_order;
      QString last_filter_term;
#if (QT_VERSION <= QT_VERSION_CHECK(5, 14, 0))
      mutable QMutex loading_mutex{QMutex::Recursive};
#else
      mutable QRecursiveMutex loading_mutex;
#endif
    };
  }
}

#endif // DIRECTORYDATAMODEMPD_H
