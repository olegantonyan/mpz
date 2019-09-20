#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include "track.h"

#include <QVector>
#include <QAbstractTableModel>


namespace PlaylistUi {
  class Model : public QAbstractTableModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setTracks(const QVector<Track> &tracks);

  private:
    QVector<Track> tracks;
  };
}

#endif // PLAYLISTMODEL_H
