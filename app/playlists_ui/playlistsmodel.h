#ifndef PLAYLISTSDATAMODEL_H
#define PLAYLISTSDATAMODEL_H

#include "playlist.h"

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>

namespace PlaylistsUi {
  class Model : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    QModelIndex append(const Playlist &item);
    void remove(const QModelIndex &index);
    Playlist itemAt(const QModelIndex &index) const;
    Playlist repalceAt(const QModelIndex &index, const Playlist &newItem);
    int listSize() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  private:
    QList<Playlist> list;
  };
}

#endif // PLAYLISTSDATAMODEL_H
