#include "playlistsmodel.h"
#include "playlist/mpdloader.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFont>

namespace PlaylistsUi {
  namespace Mpd {
  Model::Model(Config::Local &conf, MpdClient::Client &cl, QObject *parent) : PlaylistsUi::Model(conf, parent), client(cl) {
      connect(&client, &MpdClient::Client::playlistUpdated, this, &Model::loadAsync);
    }

  QVariant Model::data(const QModelIndex &index, int role) const {
    auto pl = list.at(index.row());
    if (pl && pl->name() == highlight_name) {
      highlight_uid = pl->uid();
    }
    return PlaylistsUi::Model::data(index, role);
  }

    void Model::loadAsync() {
      (void)QtConcurrent::run(QThreadPool::globalInstance(), [this]() -> void {
        auto loadedList = loadMpdPlaylists();
        QMetaObject::invokeMethod(this, [this, loadedList]() { applyAsyncLoadedList(loadedList); }, Qt::QueuedConnection);
      });
    }

    void Model::applyAsyncLoadedList(const QList<std::shared_ptr<Playlist::Playlist> > &loadedList) {
      QMutexLocker locker(&loading_mutex);
      beginResetModel();
      list = loadedList;
      loadPlaylistsOrder();
      sortPlaylistsByOrder();
      endResetModel();
      emit asyncLoadFinished();
      persist();
    }

    void Model::onMpdLost() {
      beginResetModel();
      list.clear();
      endResetModel();
    }

    QList<std::shared_ptr<Playlist::Playlist>> Model::loadMpdPlaylists() {
      QList<std::shared_ptr<Playlist::Playlist>> result;
      auto plsts = client.playlists();

      for (auto it : plsts) {
        auto p = std::shared_ptr<Playlist::Playlist>(new Playlist::Playlist());
        p->rename(it.path());
        result << p;
      }

      return result;
    }

    void Model::asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist) {
      if (!playlist) {
        return;
      }
      QFuture<void> future = QtConcurrent::run(QThreadPool::globalInstance(), [this, playlist]() {
        auto tracks = Playlist::MpdLoader(client).playlistTracks(playlist->name());
        playlist->load(tracks);

        QMetaObject::invokeMethod(this, [this, playlist]() {
          creating_playlist_name.clear();
          emit asyncTracksLoadFinished(playlist);
        }, Qt::QueuedConnection);
      });
      Q_UNUSED(future);
    }

    void Model::higlight(std::shared_ptr<Playlist::Playlist> playlist) {
      if (playlist == nullptr) {
        highlight_uid = 0;
        highlight_name.clear();
        emit dataChanged(buildIndex(0), buildIndex(list.size() - 1));
      } else {
        highlight_name = playlist->name();
        emit dataChanged(itemIndex(playlist), itemIndex(playlist));
      }
    }

    void Model::appendTracksToPlaylist(std::shared_ptr<Playlist::Playlist> playlist, const QVector<Track> &tracks) {
      if (!playlist || tracks.isEmpty()) {
        return;
      }
      QStringList paths;
      for (auto track : tracks) {
        if (track.isStream()) {
          paths << track.url().toString();
        } else {
          paths << track.path();
        }
      }
      client.appendSongsToPlaylist(paths, playlist->name());
      PlaylistsUi::Model::appendTracksToPlaylist(playlist, tracks);
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
        // creating_playlist_name = "";
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
      Q_ASSERT(!filepaths.isEmpty());
      Q_UNUSED(libraryDir);

      QFuture<void> future = QtConcurrent::run(QThreadPool::globalInstance(), [this, filepaths]() {
        const QString playlistName = createPlaylistFromDirs(filepaths);

        QMetaObject::invokeMethod(this,
          [this, playlistName]() {
            creating_playlist_name = playlistName;
            order << playlistName;
          },
          Qt::QueuedConnection);
      });
      Q_UNUSED(future);
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
      return client.currentUrl().toString();
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
      QStringList names;
      for (auto path : filepaths) {
        names << path.path();
      }
      auto songs = client.lsDirsSongs(names);
      QString playlist_name = playlistUniqueName(names.join(", "));
      playlist_name.replace("/", " ∕ "); // mpd does not support slashes in playlist name, replace with U+2215 (DIVISION SLASH)

      auto optimistic_tracks = Playlist::MpdLoader(client).builTracksFromSongsSorted(songs, playlist_name);
      QStringList songs_paths;
      for (auto it : optimistic_tracks) {
        songs_paths << it.path();
      }

      client.createPlaylist(songs_paths, playlist_name);

      return playlist_name;
    }

    void Model::remove(const QModelIndex &index) {
      auto pl = itemAt(index);
      if (!pl) {
        return;
      }
      order.removeAll(pl->name());

      client.removePlaylist(pl->name());
      PlaylistsUi::Model::remove(index);
    }
  }
}
