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

    QModelIndex buildIndex(int row) const;
    QModelIndex append(std::shared_ptr<Playlist::Playlist> item);
    void remove(const QModelIndex &index);
    std::shared_ptr<Playlist::Playlist> itemAt(const QModelIndex &index) const;
    std::shared_ptr<Playlist::Playlist> itemBy(quint64 uid) const;
    std::shared_ptr<Playlist::Playlist> itemByTrack(quint64 track_uid) const;
    int listSize() const;
    QModelIndex itemIndex(std::shared_ptr<Playlist::Playlist> playlist) const;

    bool persist();

    QList<std::shared_ptr<Playlist::Playlist>> itemList() const;

    void loadAsync();

    void higlight(std::shared_ptr<Playlist::Playlist> playlist);

  signals:
    void asynLoadFinished();

  private:
    QList<std::shared_ptr<Playlist::Playlist>> list;
    Config::Local &local_conf;

    quint64 highlight_uid;
  };
}

#endif // PLAYLISTSDATAMODEL_H
