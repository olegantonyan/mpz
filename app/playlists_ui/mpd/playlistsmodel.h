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
      void loadAsync() override;
      QModelIndex currentPlaylistIndex() const override;
      void saveCurrentPlaylistIndex(const QModelIndex &idx) override;
      void createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) override;

    private:
      MpdConnection &connection;
    };
  }
}

#endif // PLAYLISTSDATAMODELMPD_H
