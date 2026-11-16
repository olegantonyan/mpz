#include "playlistmodel.h"

#include <QDataStream>
#include <QDebug>
#include <QMimeData>

namespace PlaylistUi {
  namespace Mpzd {
    Model::Model(QStyle *stl, const ColumnsConfig &col_cfg, MpzdClient::Client &cl, QObject *parent) : PlaylistUi::Model(stl, col_cfg, parent), client(cl) {
      connect(&client, &MpzdClient::Client::scanDone, this, &Model::onScanDone);
    }

    void Model::removeTracksFromPlaylist(const QList<int> &indecies) {
      auto pl = playlist();
      if (!pl) {
        return;
      }
      QStringList args{"pl_rm", QString::number(pl->uid())};
      for (int i : indecies) {
        if (i >= 0 && i < tracksSize()) {
          args << QString::number(MpzdClient::trackIdOf(trackAt(i).uid()));
        }
      }
      PlaylistUi::Model::removeTracksFromPlaylist(indecies);
      if (args.size() > 2) {
        client.request(args);
      }
    }

    QFuture<void> Model::addPaths(const QList<QDir> &filepaths, int at_row) {
      auto pl = playlist();
      if (!pl || filepaths.isEmpty()) {
        return QFuture<void>();
      }
      const quint32 id = static_cast<quint32>(pl->uid());
      QStringList args{"pl_add", QString::number(id)};
      if (at_row >= 0) {
        args << "at" << QString::number(at_row);
      }
      for (const auto &dir : filepaths) {
        args << dir.path();
      }
      QFutureInterface<void> future;
      future.reportStarted();
      client.request(args, this, [this, id, future](const MpzdClient::Reply &reply) mutable {
        if (!reply.ok) {
          qWarning() << "mpzd pl_add:" << reply.message;
          future.reportFinished();
          return;
        }
        pending_scans[id].append(future);
      });
      return future.future();
    }

    QFuture<void> Model::appendToPlaylistAsync(const QList<QDir> &filepaths) {
      return addPaths(filepaths, -1);
    }

    QFuture<void> Model::insertTracksAsync(const QList<QDir> &filepaths, int atRow) {
      return addPaths(filepaths, qMax(0, atRow));
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

    void Model::sortBy(const QString &criteria) {
      auto pl = playlist();
      if (!pl) {
        return;
      }
      PlaylistUi::Model::sortBy(criteria);
      client.request({"pl_sort", QString::number(pl->uid()), criteria});
    }

    bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
      auto pl = playlist();
      const QString format = mimeTypes().value(0);
      if (!pl || !data || !data->hasFormat(format)) {
        return PlaylistUi::Model::dropMimeData(data, action, row, column, parent);
      }
      QByteArray bytes = data->data(format);
      QDataStream stream(&bytes, QIODevice::ReadOnly);
      qint32 count = 0;
      stream >> count;
      QList<int> rows;
      for (qint32 i = 0; i < count; i++) {
        qint32 r = -1;
        stream >> r;
        if (r >= 0 && r < tracksSize()) {
          rows << r;
        }
      }
      std::sort(rows.begin(), rows.end());
      QList<quint64> uids;
      for (int r : std::as_const(rows)) {
        uids << trackAt(r).uid();
      }

      const bool result = PlaylistUi::Model::dropMimeData(data, action, row, column, parent);

      if (!uids.isEmpty()) {
        const int pos = indexOf(uids.first()).row();
        if (pos >= 0) {
          QStringList args{"pl_move", QString::number(pl->uid()), QString::number(pos)};
          for (quint64 uid : std::as_const(uids)) {
            args << QString::number(MpzdClient::trackIdOf(uid));
          }
          client.request(args);
        }
      }
      return result;
    }

    void Model::onMpzdLost() {
      setPlaylist(nullptr);
      for (auto &futures : pending_scans) {
        for (auto &f : futures) {
          f.reportFinished();
        }
      }
      pending_scans.clear();
    }
  }
}
