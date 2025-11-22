#include "playlistsmodel.h"
#include "playlist/mpdloader.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFont>

namespace PlaylistsUi {
  namespace Mpd {
    Model::Model(Config::Local &conf, MpdConnection &conn, QObject *parent) : PlaylistsUi::Model(conf, parent), connection(conn) {
      connect(&connection, &MpdConnection::playlistUpdated, this, &Model::loadAsync);
    }

    void Model::loadAsync() {
      (void)QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
        QMutexLocker locker(&loading_mutex);
        beginResetModel();
        list = loadMpdPlaylists();
        loadPlaylistsOrder();
        sortPlaylistsByOrder();
        endResetModel();
        emit asyncLoadFinished();
        persist();
      });
    }

    void Model::onMpdLost() {
      beginResetModel();
      list.clear();
      endResetModel();
    }

    QList<std::shared_ptr<Playlist::Playlist>> Model::loadMpdPlaylists() {
      MpdConnectionLocker locker(connection);
      QList<std::shared_ptr<Playlist::Playlist>> result;
      if (!connection.conn) {
        return result;
      }

      if (!mpd_send_list_playlists(connection.conn)) {
        qWarning() << "mpd_send_list_playlists:" << connection.lastError();
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
      (void)QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
        auto tracks = Playlist::MpdLoader(connection).playlistTracks(playlist->name());
        playlist->load(tracks);
        emit asyncTracksLoadFinished(playlist);
      });
    }

    bool Model::persist() {
      auto map = local_conf.mpdPlaylistsOrder();
      auto library_path = currentLibraryPath();
      if (library_path.isEmpty()) {
        return false;
      }
      QList<QString> names;
      for (auto it : list) {
        if (it) {
          names << it->name();
        }
      }
      map[library_path] = names;
      return local_conf.saveMpdPlaylistsOrder(map);
    }

    QModelIndex Model::indexByName(const QString &name) const {
      for (int i = 0; i < list.size(); i++) {
        auto pl = list.at(i);
        if (pl && pl->name() == name) {
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
      if (currentLibraryPath().isEmpty()) {
        return QModelIndex();
      }

      auto names = local_conf.currentMpdPlaylist();
      auto name = names.value(currentLibraryPath());
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
      if (!currentLibraryPath().isEmpty()) {
        auto names = local_conf.currentMpdPlaylist();
        names[currentLibraryPath()] = pl->name();
        local_conf.saveCurrentMpdPlaylist(names);
      }
    }

    void Model::createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) {
      Q_ASSERT(filepaths.size() > 0);
      Q_UNUSED(libraryDir);

      (void)QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
        creating_playlist_name = createPlaylistFromDirs(filepaths);
        order << creating_playlist_name;
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

    QString Model::currentLibraryPath() const {
      return connection.currentUrl().toString();
    }

    void Model::sortPlaylistsByOrder() {
      QHash<QString, int> rank;
      rank.reserve(order.size());

      for (int i = 0; i < order.size(); ++i) {
        rank.insert(order[i], i);
      }

      std::sort(list.begin(), list.end(), [&](auto a, auto b) {
        return rank.value(a->name(), INT_MAX) < rank.value(b->name(), INT_MAX);
      });
    }

    void Model::loadPlaylistsOrder() {
      if (!currentLibraryPath().isEmpty()) {
        order = local_conf.mpdPlaylistsOrder()[currentLibraryPath()];
      }
    }

    QString Model::createPlaylistFromDirs(const QList<QDir> &filepaths) {
      MpdConnectionLocker locker(connection);

      QStringList songs;
      QStringList names;
      for (auto path : filepaths) {
        names << path.path();

        if (!mpd_send_list_all_meta(connection.conn, path.path().toUtf8().constData())) {
          qWarning() << "mpd_send_list_all_meta: " << connection.lastError();
          mpd_response_finish(connection.conn);
          return "";
        }

        struct mpd_song *song;
        while ((song = mpd_recv_song(connection.conn)) != nullptr) {
          const char *uri = mpd_song_get_uri(song);
          songs << uri;
          mpd_song_free(song);
        }

        mpd_response_finish(connection.conn);
      }
      QString result = playlistUniqueName(names.join(", "));

      mpd_run_clear(connection.conn);

      for (auto song : songs) {
        if (!mpd_run_add(connection.conn, song.toUtf8().constData())) {
          qWarning() << "mpd_run_add: " << connection.lastError();
          return "";
        }
      }

      if (!mpd_run_save(connection.conn, result.toUtf8().constData())) {
        qWarning() << "mpd_run_save: " << connection.lastError();
        return "";
      }

      return result;
    }

    void Model::remove(const QModelIndex &index) {
      auto pl = itemAt(index);
      if (!pl) {
        return;
      }
      order.removeAll(pl->name());

      MpdConnectionLocker locker(connection);
      if (!mpd_run_rm(connection.conn, pl->name().toUtf8().constData())) {
        qWarning() << "error deleting mpd playlist:" << connection.lastError();
        return;
      }
      PlaylistsUi::Model::remove(index);
    }
  }
}
