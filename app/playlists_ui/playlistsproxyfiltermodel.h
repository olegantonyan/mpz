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

    std::shared_ptr<Playlist::Playlist> itemAt(const QModelIndex &index) const;

    bool persist();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  };
}

#endif // PLAYLISTSPROXYFILTERMODEL_H
