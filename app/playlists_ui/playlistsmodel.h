#ifndef PLAYLISTSDATAMODEL_H
#define PLAYLISTSDATAMODEL_H

#include "playlist.h"

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>
#include <memory>

namespace PlaylistsUi {
  class Model : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    QModelIndex append(std::shared_ptr<Playlist> item);
    void remove(const QModelIndex &index);
    std::shared_ptr<Playlist> itemAt(const QModelIndex &index) const;
    int listSize() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  private:
    QList<std::shared_ptr<Playlist>> list;
  };
}

#endif // PLAYLISTSDATAMODEL_H
