#include "playlistmodel.h"

#include <QDebug>
#include <QtConcurrent>

namespace PlaylistUi {
  namespace Mpd {
    Model::Model(QStyle *stl, const ColumnsConfig &col_cfg, MpdConnection &conn, QObject *parent) :
      PlaylistUi::Model(stl, col_cfg, parent),
      connection(conn) {
    }

    void Model::reload() {
      PlaylistUi::Model::reload();
    }

    void Model::remove(const QList<QModelIndex> &items) {
    }

    void Model::appendToPlaylistAsync(const QList<QDir> &filepaths) {
      if (!playlist()) {
        return;
      }

      (void)QtConcurrent::run([=]() {
        for (auto path : filepaths) {
          //Playlist::MpdLoader loader(path, connection);
          //playlist()->append(loader.tracks(), !loader.is_playlist_file());
        }

        emit appendToPlaylistAsyncFinished(playlist());
      });
    }
  }
}
