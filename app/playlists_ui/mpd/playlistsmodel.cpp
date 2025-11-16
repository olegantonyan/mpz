#include "playlistsmodel.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFont>
#include <QMutexLocker>

namespace PlaylistsUi {
  namespace Mpd {
    Model::Model(Config::Local &conf, MpdConnection &conn, QObject *parent) : PlaylistsUi::Model(conf, parent), connection(conn) {
      connect(&conn, &MpdConnection::playlist_updated, this, &Model::loadAsync);
    }

    void Model::loadAsync() {
      (void)QtConcurrent::run([=]() {
        beginResetModel();
        list = loadMpdPlaylists();
        endResetModel();
        emit asyncLoadFinished();
        persist();
      });
    }

    QList<std::shared_ptr<Playlist::Playlist>> Model::loadMpdPlaylists() {
      QMutexLocker locker(&connection.mutex);
      QList<std::shared_ptr<Playlist::Playlist>> result;

      if (!mpd_send_list_playlists(connection.conn)) {
        qWarning() << "mpd_send_list_playlists:" << connection.last_error();
        return result;
      }

      struct mpd_playlist *pl;
      while ((pl = mpd_recv_playlist(connection.conn)) != nullptr) {
        auto p = std::shared_ptr<Playlist::Playlist>(new Playlist::Playlist());
        auto name = mpd_playlist_get_path(pl);
        if (name) {
          p->rename(name);
          result << p;
        }
        mpd_playlist_free(pl);
      }
      mpd_response_finish(connection.conn);

      return result;
    }

    void Model::asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist) {
      if (!playlist) {
        return;
      }
      (void)QtConcurrent::run([=]() {
        playlist->load(loadPlaylistTracks(playlist->name()));
        emit asyncTracksLoadFinished(playlist);
      });
    }

    QVector<Track> Model::loadPlaylistTracks(const QString &name) {
      QMutexLocker locker(&connection.mutex);

      QVector<Track> result;

      if (!mpd_send_list_playlist_meta(connection.conn, name.toUtf8().constData())) {
        qWarning() << "mpd_send_list_playlist_meta:" << connection.last_error();
        return result;
      }

      struct mpd_song *song;
      while ((song = mpd_recv_song(connection.conn)) != nullptr) {
        const char *uri = mpd_song_get_uri(song);
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
        const char *genre = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
        const char *tracknum  = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
        const char *year   = mpd_song_get_tag(song, MPD_TAG_DATE, 0);

        result << Track(
          uri ? QString::fromUtf8(uri) : QString(),
          0,
          artist ? QString::fromUtf8(artist) : QString(),
          album ? QString::fromUtf8(album) : QString(),
          title ? QString::fromUtf8(title) : QString(),
          tracknum ? QString::fromUtf8(tracknum).toInt() : 0,
          year ? QString::fromUtf8(year).toInt() : 0,
          mpd_song_get_duration(song) * 1000,
          0,
          0,
          0
        );

        mpd_song_free(song);
      }

      mpd_response_finish(connection.conn);

      return result;
    }

    bool Model::persist() {
      return true; //local_conf.savePlaylists(list);
    }

    QModelIndex Model::currentPlaylistIndex() const {
      return QModelIndex(); //buildIndex(qMin(local_conf.currentPlaylist(), listSize() - 1));
    }

    void Model::saveCurrentPlaylistIndex(const QModelIndex &idx) {
      /*int current_index = idx.row();
      auto max_index = qMax(listSize() - 1, 0);
      auto save_index = qMin(current_index, max_index);
      local_conf.saveCurrentPlaylist(save_index);*/
    }

    void Model::createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) {
      Q_ASSERT(filepaths.size() > 0);

      auto pl = std::shared_ptr<Playlist::Playlist>(new Playlist::Playlist());

      (void)QtConcurrent::run([=]() {
        QStringList names;
        for (auto path : filepaths) {
          //Playlist::Loader loader(path);
          //pl->append(loader.tracks(), !loader.is_playlist_file());
          names << playlistNameBy(path, libraryDir);
        }
        pl->rename(names.join(", "));
        emit createPlaylistAsyncFinished(pl);
      });
    }

    void Model::remove(const QModelIndex &index) {
      auto pl = itemAt(index);
      if (!pl) {
        return;
      }

      QMutexLocker locker(&connection.mutex);
      if (!mpd_run_rm(connection.conn, pl->name().toUtf8().constData())) {
        qWarning() << "error deleting mpd playlist:" << connection.last_error();
        return;
      }
      PlaylistsUi::Model::remove(index);
    }
  }
}
