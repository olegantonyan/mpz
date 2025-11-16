#include "playlistsmodel.h"
#include "playlist/mpdloader.h"

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
        Playlist::MpdLoader loader(playlist->name(), connection);
        playlist->load(loader.tracks());
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

    QModelIndex Model::indexByName(const QString &name) const {
      for (int i = 0; i < list.size(); i++) {
        auto pl = list.at(i);
        if (pl->name() == name) {
          return buildIndex(i);
        }
      }
      return QModelIndex();
    }

    QModelIndex Model::currentPlaylistIndex() {
      if (!creating_playlist_name.isEmpty()) {
        auto index = indexByName(creating_playlist_name);
        creating_playlist_name = "";
        return index;
      }
      auto name = local_conf.currentMpdPlaylist();
      if (name.isEmpty()) {
        return QModelIndex();
      }
      return indexByName(name);
    }

    void Model::saveCurrentPlaylistIndex(const QModelIndex &idx) {
      auto pl = itemAt(idx);
      if (!pl) {
        return;
      }
      local_conf.saveCurrentMpdPlaylist(pl->name());
    }

    void Model::createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) {
      Q_ASSERT(filepaths.size() > 0);

      (void)QtConcurrent::run([=]() {
        creating_playlist_name = createPlaylistFromDirs(filepaths, libraryDir);
      });
    }

    QString Model::playlistUniqueName(const QString &name) const {
      QString result = name;
      if (indexByName(result).isValid()) {
        QString base_name = result;
        QStringList existing;
        for (auto pl : list) {
          existing << pl->name();
        }
        int suffix = 1;
        while (existing.contains(result)) {
          result = QString("%1 (%2)").arg(base_name).arg(suffix++);
        }
      }
      return result;
    }

    QString Model::createPlaylistFromDirs(const QList<QDir> &filepaths, const QString &libraryDir) {
      QMutexLocker locker(&connection.mutex);

      QStringList songs;
      QStringList names;
      for (auto path : filepaths) {
        names << path.path();

        QByteArray path_bytes = path.path().toUtf8();
        if (!mpd_send_list_all_meta(connection.conn, path_bytes.constData())) {
          qWarning() << "mpd_send_list_all_meta: " << connection.last_error();
          mpd_response_finish(connection.conn);
          return "";
        }

        struct mpd_song *song;
        while ((song = mpd_recv_song(connection.conn)) != NULL) {
          const char *uri = mpd_song_get_uri(song);
          songs << uri;
          mpd_song_free(song);
        }

        mpd_response_finish(connection.conn);
      }
      QString result = playlistUniqueName(names.join(", "));
      QByteArray result_bytes = result.toUtf8();

      mpd_run_clear(connection.conn);

      for (auto song : songs) {
        QByteArray song_bytes = song.toUtf8();
        if (!mpd_run_add(connection.conn, song_bytes.constData())) {
          qWarning() << "mpd_run_add: " << connection.last_error();
          return "";
        }
      }

      if (!mpd_run_save(connection.conn, result_bytes.constData())) {
        qWarning() << "mpd_run_save: " << connection.last_error();
        return "";
      }

      return result;
    }

    void Model::remove(const QModelIndex &index) {
      auto pl = itemAt(index);
      if (!pl) {
        return;
      }

      QMutexLocker locker(&connection.mutex);
      QByteArray name_bytes = pl->name().toUtf8();
      if (!mpd_run_rm(connection.conn, name_bytes.constData())) {
        qWarning() << "error deleting mpd playlist:" << connection.last_error();
        return;
      }
      PlaylistsUi::Model::remove(index);
    }
  }
}
