#ifndef PLAYLISTSDATAMODEL_H
#define PLAYLISTSDATAMODEL_H

#include "playlist/playlist.h"
#include "config/local.h"

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>
#include <memory>

namespace PlaylistsUi {
  class Model : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit Model(Config::Local &conf, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    QModelIndex buildIndex(int row) const;
    QModelIndex append(std::shared_ptr<Playlist::Playlist> item);
    virtual void remove(const QModelIndex &index);
    std::shared_ptr<Playlist::Playlist> itemAt(const QModelIndex &index) const;
    std::shared_ptr<Playlist::Playlist> itemBy(quint64 uid) const;
    std::shared_ptr<Playlist::Playlist> itemByTrack(quint64 track_uid) const;
    int listSize() const;
    QModelIndex itemIndex(std::shared_ptr<Playlist::Playlist> playlist) const;
    virtual bool persist();
    QList<std::shared_ptr<Playlist::Playlist>> itemList() const;
    virtual void higlight(std::shared_ptr<Playlist::Playlist> playlist);
    virtual QModelIndex currentPlaylistIndex();
    virtual void saveCurrentPlaylistIndex(const QModelIndex &idx);
    virtual void createPlaylistAsync(const QList<QDir> &filepaths, const QString &libraryDir);
    // Tracks are already built (radio stations), so there is nothing to scan.
    void createPlaylistFromTracks(const QVector<Track> &tracks, const QString &name);
    virtual void asyncTracksLoad(std::shared_ptr<Playlist::Playlist> playlist);
    virtual void appendTracksToPlaylist(std::shared_ptr<Playlist::Playlist> playlist, const QVector<Track> &tracks);

  public slots:
    virtual void loadAsync();

  signals:
    void asyncLoadFinished();
    void createPlaylistAsyncFinished(std::shared_ptr<Playlist::Playlist> playlist);
    void asyncTracksLoadFinished(std::shared_ptr<Playlist::Playlist> playlist);

  protected:
    QString playlistNameBy(const QDir &path, const QString &libraryDir = "");

    QList<std::shared_ptr<Playlist::Playlist>> list;
    Config::Local &local_conf;

    quint64 highlight_uid = 0;
  };
}

#endif // PLAYLISTSDATAMODEL_H
