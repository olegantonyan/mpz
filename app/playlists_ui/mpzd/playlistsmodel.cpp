#include "playlistsmodel.h"

#include <QDebug>

namespace PlaylistsUi {
  namespace Mpzd {
    Model::Model(Config::Local &conf, MpzdClient::Client &cl, QObject *parent) : PlaylistsUi::Model(conf, parent), client(cl) {
      connect(&client, &MpzdClient::Client::playlistsChanged, this, &Model::loadAsync);
      connect(&client, &MpzdClient::Client::playlistChanged, this, &Model::onPlaylistChanged);
      connect(&client, &MpzdClient::Client::scanDone, this, &Model::onScanDone);
    }

    void Model::loadAsync() {
      if (!client.isConnected()) {
        emit asyncLoadFinished();
        return;
      }
      client.request({"playlists"}, this, [this](const MpzdClient::Reply &reply) {
        if (reply.ok) {
          applyPlaylists(MpzdClient::playlistsFromReply(reply));
        } else {
          emit asyncLoadFinished();
        }
      });
    }

    void Model::applyPlaylists(const QList<MpzdClient::PlaylistInfo> &infos) {
      QList<std::shared_ptr<Playlist::Playlist>> fresh;
      bool structural = !loaded_once || infos.size() != list.size();
      for (int i = 0; i < infos.size(); i++) {
        const auto &info = infos.at(i);
        auto p = itemBy(info.id);
        if (!p) {
          p = std::make_shared<Playlist::Playlist>();
          p->setUid(info.id);
        }
        p->rename(info.name);
        p->setRandom(MpzdClient::playlistOrderFrom(info.order));
        if (!structural && list.at(i)->uid() != info.id) {
          structural = true;
        }
        fresh << p;
      }
      if (!loaded_once && current_id == 0) {
        current_id = client.status().sel_playlist;
      }
      if (structural) {
        beginResetModel();
        list = fresh;
        endResetModel();
        remember();
        loaded_once = true;
        emit asyncLoadFinished();
      } else {
        list = fresh;
        remember();
        if (!list.isEmpty()) {
          emit dataChanged(buildIndex(0), buildIndex(list.size() - 1));
        }
      }
    }

    void Model::remember() {
      known.clear();
      known_order.clear();
      for (const auto &p : std::as_const(list)) {
        known.insert(p->uid(), {p->name(), p->random()});
        known_order << p->uid();
      }
    }

    void Model::onMpzdLost() {
      beginResetModel();
      list.clear();
      endResetModel();
      known.clear();
      known_order.clear();
      loaded_once = false;
      current_id = 0;
      shown_id = 0;
      for (auto &futures : pending_scans) {
        for (auto &f : futures) {
          f.reportFinished();
        }
      }
      pending_scans.clear();
    }

    bool Model::persist() {
      if (!client.isConnected()) {
        return false;
      }
      for (const auto &p : std::as_const(list)) {
        if (!known.contains(p->uid())) {
          continue;
        }
        const auto &k = known[p->uid()];
        const QString id = QString::number(p->uid());
        if (k.name != p->name()) {
          client.request({"pl_rename", id, p->name()});
        }
        if (k.order != p->random()) {
          client.request({"pl_order", id, MpzdClient::playlistOrderName(p->random())});
        }
      }
      QList<quint64> sim;
      for (quint64 uid : std::as_const(known_order)) {
        if (itemBy(uid)) {
          sim << uid;
        }
      }
      for (int i = 0; i < list.size() && i < sim.size(); i++) {
        const quint64 uid = list.at(i)->uid();
        if (sim.at(i) != uid) {
          const int from = sim.indexOf(uid);
          if (from >= 0) {
            sim.move(from, i);
            client.request({"pl_pos", QString::number(uid), QString::number(i)});
          }
        }
      }
      remember();
      return true;
    }

    void Model::remove(const QModelIndex &index) {
      auto pl = itemAt(index);
      if (!pl) {
        return;
      }
      client.request({"pl_del", QString::number(pl->uid())});
      PlaylistsUi::Model::remove(index);
    }

    QModelIndex Model::currentPlaylistIndex() {
      for (int i = 0; i < list.size(); i++) {
        if (list.at(i)->uid() == current_id) {
          return buildIndex(i);
        }
      }
      return list.isEmpty() ? QModelIndex() : buildIndex(0);
    }

    void Model::saveCurrentPlaylistIndex(const QModelIndex &idx) {
      auto pl = itemAt(idx);
      if (!pl || pl->uid() == current_id) {
        return;
      }
      current_id = pl->uid();
      client.request({"select", QString::number(current_id)});
    }

    void Model::trackScan(quint32 id, QFutureInterface<void> future) {
      pending_scans[id].append(future);
    }

    void Model::onScanDone(quint32 id) {
      auto it = pending_scans.find(id);
      if (it == pending_scans.end() || it->isEmpty()) {
        return;
      }
      auto future = it->takeFirst();
      future.reportFinished();
      if (it->isEmpty()) {
        pending_scans.erase(it);
      }
    }

    QFuture<void> Model::createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir) {
      Q_UNUSED(libraryDir);
      QFutureInterface<void> future;
      future.reportStarted();
      QStringList args{"pl_new", "-"};
      for (const auto &dir : filepaths) {
        args << dir.path();
      }
      client.request(args, this, [this, future](const MpzdClient::Reply &reply) mutable {
        if (!reply.ok) {
          qWarning() << "mpzd pl_new:" << reply.message;
          future.reportFinished();
          return;
        }
        const quint32 id = reply.value("playlist").toUInt();
        current_id = id;
        trackScan(id, future);
      });
      return future.future();
    }

    void Model::createPlaylistFromTracks(const QVector<Track> &tracks, const QString &name) {
      if (tracks.isEmpty()) {
        return;
      }
      QStringList args{"pl_new", name.isEmpty() ? "-" : name};
      for (const auto &t : tracks) {
        args << (t.isStream() ? t.url().toString() : t.path());
      }
      client.request(args, this, [this](const MpzdClient::Reply &reply) {
        if (reply.ok) {
          current_id = reply.value("playlist").toUInt();
        }
      });
    }

    void Model::asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist) {
      if (!playlist) {
        return;
      }
      shown_id = playlist->uid();
      client.fetchTracks(static_cast<quint32>(playlist->uid()), playlist->name(), this, [this, playlist](bool ok, const QVector<Track> &tracks) {
        if (ok) {
          playlist->load(tracks);
        }
        if (shown_id == playlist->uid()) {
          emit asyncTracksLoadFinished(playlist);
        }
      });
    }

    void Model::onPlaylistChanged(quint32 id) {
      if (id != shown_id) {
        return;
      }
      auto pl = itemBy(id);
      if (pl) {
        asyncTracksLoad(pl);
      }
    }

    void Model::appendTracksToPlaylist(std::shared_ptr<Playlist::Playlist> playlist, const QVector<Track> &tracks) {
      if (!playlist || tracks.isEmpty()) {
        return;
      }
      const QString dst = QString::number(playlist->uid());
      QMap<quint32, QStringList> copies;
      QStringList adds{"pl_add", dst};
      for (const auto &t : tracks) {
        if (t.isRemote() && t.remote_server_url() == client.currentUrl() && MpzdClient::playlistIdOf(t.uid()) != 0) {
          copies[MpzdClient::playlistIdOf(t.uid())] << QString::number(MpzdClient::trackIdOf(t.uid()));
        } else {
          adds << (t.isStream() ? t.url().toString() : t.path());
        }
      }
      for (auto it = copies.cbegin(); it != copies.cend(); ++it) {
        client.request(QStringList{"pl_copy", QString::number(it.key()), dst} + it.value());
      }
      if (adds.size() > 2) {
        client.request(adds);
      }
    }

    void Model::reloadPlaylist(std::shared_ptr<Playlist::Playlist> playlist) {
      if (playlist) {
        client.request({"pl_reload", QString::number(playlist->uid())});
      }
    }

    std::shared_ptr<Playlist::Playlist> Model::itemByTrack(quint64 track_uid) const {
      return itemBy(MpzdClient::playlistIdOf(track_uid));
    }
  }
}
