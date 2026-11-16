#ifndef PLAYLISTMODELMPZD_H
#define PLAYLISTMODELMPZD_H

#include "playlist_ui/playlistmodel.h"
#include "mpzd_client/client.h"

#include <QFutureInterface>
#include <QHash>
#include <QObject>

namespace PlaylistUi {
  namespace Mpzd {
    class Model : public PlaylistUi::Model {
      Q_OBJECT
    public:
      explicit Model(QStyle *stl, const ColumnsConfig &col_cfg, MpzdClient::Client &cl, QObject *parent = nullptr);

      QFuture<void> appendToPlaylistAsync(const QList<QDir> &filepaths) override;
      QFuture<void> insertTracksAsync(const QList<QDir> &filepaths, int atRow) override;
      void sortBy(const QString &criteria) override;
      bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    public slots:
      void onMpzdLost();

    protected:
      void removeTracksFromPlaylist(const QList<int> &indecies) override;

    private:
      MpzdClient::Client &client;
      QHash<quint32, QList<QFutureInterface<void>>> pending_scans;

      QFuture<void> addPaths(const QList<QDir> &filepaths, int at_row);
      void onScanDone(quint32 id);
    };
  }
}

#endif // PLAYLISTMODELMPZD_H
