#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include "track.h"
#include "playlist.h"

#include <QVector>
#include <QAbstractTableModel>
#include <memory>
#include <QList>

namespace PlaylistUi {
  class Model : public QAbstractTableModel {
    Q_OBJECT

  public:
    explicit Model(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setPlaylist(std::shared_ptr<Playlist> pl);

    Track itemAt(const QModelIndex &index) const;
    QModelIndex buildIndex(int row) const;
    int tracksSize() const;
    void highlight(quint64 uid);
    QModelIndex indexOf(quint64 uid) const;

    std::shared_ptr<Playlist> playlist();
    void reload();

    void remove(const QList<QModelIndex> &items);

  private:
    void setTracks(const QVector<Track> &tracks);

    std::shared_ptr<Playlist> _playlist;
    QVector<Track> tracks;
    quint64 highlight_uid;
  };
}

#endif // PLAYLISTMODEL_H
