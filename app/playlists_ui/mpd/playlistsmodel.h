#ifndef PLAYLISTSDATAMODELMPD_H
#define PLAYLISTSDATAMODELMPD_H

#include "playlist/playlist.h"
#include "config/local.h"
#include "playlists_ui/playlistsmodel.h"
#include "mpdconnection.h"

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>
#include <memory>

namespace PlaylistsUi {
  namespace Mpd {
    class Model : public PlaylistsUi::Model {
      Q_OBJECT

    public:
      explicit Model(Config::Local &conf, MpdConnection &conn, QObject *parent = nullptr);

      bool persist() override;
      void remove(const QModelIndex &index) override;
      QModelIndex currentPlaylistIndex() override;
      void saveCurrentPlaylistIndex(const QModelIndex &idx) override;
      void createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) override;
      void asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist) override;

    public slots:
      void loadAsync() override;

    private:
      MpdConnection &connection;
      QString creating_playlist_name;

      QList<std::shared_ptr<Playlist::Playlist>> loadMpdPlaylists();
      QString createPlaylistFromDirs(const QList<QDir> &filepaths, const QString &libraryDir);
      QModelIndex indexByName(const QString &name) const;
      QString playlistUniqueName(const QString &name) const;
    };
  }
}

#endif // PLAYLISTSDATAMODELMPD_H
