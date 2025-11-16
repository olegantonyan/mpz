#include "playlistmodel.h"
#include "playlist/mpdloader.h"

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
        auto tracks = Playlist::MpdLoader(connection).dirsTracks(filepaths, playlist()->name());
        playlist()->append(tracks, true);
        appendToPlaylist(tracks, playlist()->name());
        emit appendToPlaylistAsyncFinished(playlist());
      });
    }

    bool Model::appendToPlaylist(const QVector<Track> &tracks, const QString &playlist_name) {
      QMutexLocker locker(&connection.mutex);

      if (!mpd_command_list_begin(connection.conn, true)) {
        qWarning() << "mpd_send_list_all_meta: " << connection.last_error();
        return false;
      }

      bool ok = true;
      for (auto track : tracks) {
        if (!mpd_send_playlist_add(connection.conn, playlist_name.toUtf8().constData(), track.path().toUtf8().constData())) {
          qWarning() << "mpd_send_playlist_add: " << connection.last_error();
          ok = false;
        }
      }

      if (!mpd_command_list_end(connection.conn)) {
        qWarning() << "mpd_send_playlist_add: " << connection.last_error();
        ok = false;
      }

      if (!mpd_response_finish(connection.conn)) {
        qWarning() << "mpd_send_playlist_add: " << connection.last_error();
        ok = false;
      }

      return ok;
    }
  }
}
