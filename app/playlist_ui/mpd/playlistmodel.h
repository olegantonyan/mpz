#ifndef PLAYLISTMODELMPD_H
#define PLAYLISTMODELMPD_H

#include "track.h"
#include "playlist/playlist.h"
#include "playlist_ui/columnsconfig.h"
#include "playlist_ui/playlistmodel.h"
#include "mpdconnection.h"

#include <QVector>
#include <QAbstractTableModel>
#include <memory>
#include <QList>
#include <QStyle>

namespace PlaylistUi {
  namespace Mpd {
    class Model : public PlaylistUi::Model {
      Q_OBJECT

    public:
      explicit Model(QStyle *stl, const ColumnsConfig &col_cfg, MpdConnection &conn, QObject *parent = nullptr);

      void reload() override;
      void appendToPlaylistAsync(const QList<QDir> &filepaths) override;

    public slots:
      void onMpdLost();

    protected:
      void removeTracksFromPlaylist(const QList<int> &indecies) override;

    private:
      MpdConnection &connection;

      bool appendToPlaylist(const QVector<Track> &tracks, const QString &playlist_name);
    };
  }
}

#endif // PLAYLISTMODELMPD_H
