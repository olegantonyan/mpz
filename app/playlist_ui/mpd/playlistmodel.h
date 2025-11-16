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

    private:
      MpdConnection &connection;
    };
  }
}

#endif // PLAYLISTMODELMPD_H
