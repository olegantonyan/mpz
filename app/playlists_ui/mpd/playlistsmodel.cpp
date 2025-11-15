#include "playlistsmodel.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFont>

namespace PlaylistsUi {
  namespace Mpd {
    Model::Model(Config::Local &conf, MpdConnection &conn, QObject *parent) : PlaylistsUi::Model(conf, parent), connection(conn) {
    }

    void Model::loadAsync() {
      (void)QtConcurrent::run([=]() {
        beginResetModel();
        //list = local_conf.playlists();
        endResetModel();
        emit asyncLoadFinished();
        persist();
      });
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
  }
}
