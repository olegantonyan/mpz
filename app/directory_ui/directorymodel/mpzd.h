#ifndef DIRECTORYDATAMODELMPZD_H
#define DIRECTORYDATAMODELMPZD_H

#include "mpzd_client/client.h"
#include "mpd/treeitem.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

namespace DirectoryUi {
  namespace DirectoryModel {
    class Mpzd : public QAbstractItemModel {
      Q_OBJECT

    signals:
      void directoryLoaded(const QString &path);

    public:
      explicit Mpzd(MpzdClient::Client &cl, QObject *parent = nullptr);
      ~Mpzd();

      void loadAsync(const QString &path);
      void filter(const QString &term);
      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;

      QModelIndex index(int row, int column, const QModelIndex &parent) const override;
      QModelIndex parent(const QModelIndex &child) const override;
      int rowCount(const QModelIndex &parent) const override;
      int columnCount(const QModelIndex &parent) const override;
      QVariant data(const QModelIndex &index, int role) const override;
      bool canFetchMore(const QModelIndex &parent) const override;
      void fetchMore(const QModelIndex &parent) override;
      bool hasChildren(const QModelIndex &parent) const override;
      void sort(int column, Qt::SortOrder order) override;

    public slots:
      void onMpzdReady();
      void onMpzdLost();

    private:
      MpzdClient::Client &client;
      TreeItem *root_item = nullptr;
      quint64 generation = 0;
      QString sort_mode = "name";
      QString filter_term;

      void reload();
      void resetRoot();
      void listInto(TreeItem *item, const QString &path, std::function<void()> done = nullptr);
      TreeItem *itemFrom(const QModelIndex &index) const;
      QModelIndex indexOf(TreeItem *item) const;
    };
  }
}

#endif // DIRECTORYDATAMODELMPZD_H
