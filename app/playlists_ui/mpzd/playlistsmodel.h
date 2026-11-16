#ifndef PLAYLISTSDATAMODELMPZD_H
#define PLAYLISTSDATAMODELMPZD_H

#include "playlists_ui/playlistsmodel.h"
#include "mpzd_client/client.h"

#include <QFutureInterface>
#include <QHash>
#include <QObject>

namespace PlaylistsUi {
  namespace Mpzd {
    class Model : public PlaylistsUi::Model {
      Q_OBJECT
    public:
      explicit Model(Config::Local &conf, MpzdClient::Client &cl, QObject *parent = nullptr);

      bool persist() override;
      void remove(const QModelIndex &index) override;
      QModelIndex currentPlaylistIndex() override;
      void saveCurrentPlaylistIndex(const QModelIndex &idx) override;
      QFuture<void> createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) override;
      void createPlaylistFromTracks(const QVector<Track> &tracks, const QString &name) override;
      void asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist) override;
      void appendTracksToPlaylist(std::shared_ptr<Playlist::Playlist> playlist, const QVector<Track> &tracks) override;
      void reloadPlaylist(std::shared_ptr<Playlist::Playlist> playlist) override;
      std::shared_ptr<Playlist::Playlist> itemByTrack(quint64 track_uid) const override;

    public slots:
      void loadAsync() override;
      void onMpzdLost();

    private:
      struct Known {
        QString name;
        Playlist::Playlist::PlaylistRandom order;
      };

      MpzdClient::Client &client;
      quint64 current_id = 0;
      quint64 shown_id = 0;
      bool loaded_once = false;
      QHash<quint64, Known> known;
      QList<quint64> known_order;
      QHash<quint32, QList<QFutureInterface<void>>> pending_scans;

      void applyPlaylists(const QList<MpzdClient::PlaylistInfo> &infos);
      void remember();
      void onPlaylistChanged(quint32 id);
      void onScanDone(quint32 id);
      void trackScan(quint32 id, QFutureInterface<void> future);
    };
  }
}

#endif // PLAYLISTSDATAMODELMPZD_H
