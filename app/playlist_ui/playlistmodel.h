#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include "track.h"
#include "trackwrapper.h"

#include <QVector>
#include <QAbstractTableModel>


namespace PlaylistUi {
  class Model : public QAbstractTableModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    QModelIndex buildIndex(int row);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setTracks(const QVector<Track> &tracks, int index);

    Track itemAt(const QModelIndex &index) const;

    int current_playlist_index() const;

    int tracksSize() const;

    void highlight(int row);

  private:
    QVector<Track> tracks;
    int playlist_index;
    int highlight_row;
  };
}

#endif // PLAYLISTMODEL_H
