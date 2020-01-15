#ifndef PROXYFILTERMODEL_H
#define PROXYFILTERMODEL_H

#include <QObject>
#include <QSortFilterProxyModel>

namespace PlaylistsUi {
  class ProxyFilterModel : public QSortFilterProxyModel {
  public:
    explicit ProxyFilterModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  };
}

#endif // PROXYFILTERMODEL_H
