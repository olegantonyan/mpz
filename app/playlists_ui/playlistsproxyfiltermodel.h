#ifndef PLAYLISTSPROXYFILTERMODEL_H
#define PLAYLISTSPROXYFILTERMODEL_H

#include "playlistsmodel.h"

#include <QObject>
#include <QSortFilterProxyModel>

namespace PlaylistsUi {
  class ProxyFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit ProxyFilterModel(Config::Local &conf, QObject *parent = nullptr);

    std::shared_ptr<Playlist::Playlist> itemAt(const QModelIndex &index) const;
    QModelIndex append(std::shared_ptr<Playlist::Playlist> pl);

    bool persist();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Model *activeModel() const;

  signals:
    void asyncLoadFinished();
    void createPlaylistAsyncFinished(std::shared_ptr<Playlist::Playlist> playlist);
  };
}

#endif // PLAYLISTSPROXYFILTERMODEL_H
