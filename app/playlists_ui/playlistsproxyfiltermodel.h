#ifndef PLAYLISTSPROXYFILTERMODEL_H
#define PLAYLISTSPROXYFILTERMODEL_H

#include "playlistsmodel.h"
#include "modusoperandi.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpd/playlistsmodel.h"
#endif

#include <QObject>
#include <QSortFilterProxyModel>

namespace PlaylistsUi {
  class ProxyFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit ProxyFilterModel(Config::Local &conf, ModusOperandi &modus, QObject *parent = nullptr);

    std::shared_ptr<Playlist::Playlist> itemAt(const QModelIndex &index) const;
    QModelIndex append(std::shared_ptr<Playlist::Playlist> pl);

    bool persist();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Model *activeModel() const;

  signals:
    void asyncLoadFinished();
    void createPlaylistAsyncFinished(std::shared_ptr<Playlist::Playlist> playlist);
    void asyncTracksLoadFinished(std::shared_ptr<Playlist::Playlist> playlist);

  private slots:
    void switchTo(ModusOperandi::ActiveMode new_mode);

  private:
    ModusOperandi &modus_operandi;
    Model *localfs;
#ifdef ENABLE_MPD_SUPPORT
    Mpd::Model *mpd;
#endif
  };
}

#endif // PLAYLISTSPROXYFILTERMODEL_H
