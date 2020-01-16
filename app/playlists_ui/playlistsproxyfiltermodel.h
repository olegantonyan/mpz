#ifndef PLAYLISTSPROXYFILTERMODEL_H
#define PLAYLISTSPROXYFILTERMODEL_H

#include "playlistsmodel.h"

#include <QObject>
#include <QSortFilterProxyModel>

namespace PlaylistsUi {
  class ProxyFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit ProxyFilterModel(Model *source_model, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  };
}

#endif // PLAYLISTSPROXYFILTERMODEL_H
