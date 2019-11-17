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

    Track itemAt(const QModelIndex &index) const;
    QModelIndex buildIndex(int row);
    int tracksSize() const;
    void highlight(quint64 uid);

  private:
    QVector<Track> tracks;
    quint64 highlight_uid;
  };
}

#endif // PLAYLISTMODEL_H
