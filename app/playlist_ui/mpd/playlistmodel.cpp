#include "playlistmodel.h"
#include "playlist/mpdloader.h"

#include <QDebug>
#include <QFont>
#include <QtConcurrent>

namespace PlaylistUi {
  namespace Mpd {
    Model::Model(QStyle *stl, const ColumnsConfig &col_cfg, MpdConnection &conn, QObject *parent) :
      PlaylistUi::Model(stl, col_cfg, parent),
      connection(conn) {
    }

  }
}
