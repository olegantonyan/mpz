#include "playlistmodel.h"
#include "playlist/mpdloader.h"

#include <QDebug>
#include <QtConcurrent>

namespace PlaylistUi {
  namespace Mpd {
    Model::Model(QStyle *stl, const ColumnsConfig &col_cfg,MpdClient::Client &cl, QObject *parent) :
      PlaylistUi::Model(stl, col_cfg, parent),
      client(cl) {
    }

    void Model::reload() {
      PlaylistUi::Model::reload();
    }

    void Model::removeTracksFromPlaylist(const QList<int> &indecies) {
      PlaylistUi::Model::removeTracksFromPlaylist(indecies);
      client.removeSongsFromPlaylist(indecies.toVector(), playlist()->name());
    }

    void Model::appendToPlaylistAsync(const QList<QDir> &filepaths) {
      if (!playlist()) {
        return;
      }

      (void)QtConcurrent::run(QThreadPool::globalInstance(), [this, filepaths]() {
        auto tracks = Playlist::MpdLoader(client).dirsTracks(filepaths, playlist()->name());
        playlist()->append(tracks, true);
        appendToPlaylist(tracks, playlist()->name());
        emit appendToPlaylistAsyncFinished(playlist());
      });
    }

    void Model::sortBy(const QString &criteria) {
      playlist()->sortBy(criteria);
      client.removePlaylist(playlist()->name());
      QStringList songs_paths;
      for (auto it : playlist()->tracks()) {
        songs_paths << it.path();
      }
      client.createPlaylist(songs_paths, playlist()->name());
      reload();
    }

    void Model::onMpdLost() {
      setPlaylist(nullptr);
    }

    bool Model::appendToPlaylist(const QVector<Track> &tracks, const QString &playlist_name) {
      QStringList paths;
      for (auto track : tracks) {
        paths << track.path();
      }
      return client.appendSongsToPlaylist(paths, playlist_name);;
    }
  }
}
