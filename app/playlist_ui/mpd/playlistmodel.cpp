#include "playlistmodel.h"
#include "playlist/mpd_track.h"

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
        auto tracks = loadDirsTracks(filepaths, playlist()->name());
        playlist()->append(tracks, true);
        appendToPlaylist(tracks, playlist()->name());
        emit appendToPlaylistAsyncFinished(playlist());
      });
    }

    QVector<Track> Model::loadDirsTracks(const QList<QDir> &filepaths, const QString &playlist_name) const {
      QMutexLocker locker(&connection.mutex);

      QVector<Track> result;
      QStringList names;
      for (auto path : filepaths) {
        names << path.path();

        QByteArray path_bytes = path.path().toUtf8();
        if (!mpd_send_list_all_meta(connection.conn, path_bytes.constData())) {
          qWarning() << "mpd_send_list_all_meta: " << connection.last_error();
          mpd_response_finish(connection.conn);
          return result;
        }

        struct mpd_song *song;
        while ((song = mpd_recv_song(connection.conn)) != NULL) {
          result << Playlist::MpdTrack::build(song, playlist_name);
          
          mpd_song_free(song);
        }

        mpd_response_finish(connection.conn);
      }

      return result;
    }

    bool Model::appendToPlaylist(const QVector<Track> &tracks, const QString &playlist_name) {
      QMutexLocker locker(&connection.mutex);

      if (!mpd_command_list_begin(connection.conn, true)) {
        qWarning() << "mpd_send_list_all_meta: " << connection.last_error();
        return false;
      }

      QByteArray playlist_name_bytes = playlist_name.toUtf8();
      bool ok = true;

      for (auto track : tracks) {
        QByteArray filepath_bytes = track.path().toUtf8();
        if (!mpd_send_playlist_add(connection.conn, playlist_name_bytes.constData(), filepath_bytes.constData())) {
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
