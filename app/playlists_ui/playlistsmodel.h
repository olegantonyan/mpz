#ifndef PLAYLISTSDATAMODEL_H
#define PLAYLISTSDATAMODEL_H

#include "playlist.h"
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

    QModelIndex buildIndex(int row);
    QModelIndex append(std::shared_ptr<Playlist> item);
    void remove(const QModelIndex &index);
    std::shared_ptr<Playlist> itemAt(const QModelIndex &index) const;
    int listSize() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  private:
    QList<std::shared_ptr<Playlist>> list;
    Config::Local &local_conf;

    bool persist();
  };
}

#endif // PLAYLISTSDATAMODEL_H
