#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include "track.h"
#include "playlist/playlist.h"
#include "playlist_ui/columnsconfig.h"

#include <QVector>
#include <QAbstractTableModel>
#include <memory>
#include <QList>
#include <QStyle>

namespace PlaylistUi {
  class Model : public QAbstractTableModel {
    Q_OBJECT

  public:
    enum HighlightState {
      None,
      Playing,
      Paused
    };
    explicit Model(QStyle *stl, const ColumnsConfig &col_cfg, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setPlaylist(std::shared_ptr<Playlist::Playlist> pl);

    Track itemAt(const QModelIndex &index) const;
    QModelIndex buildIndex(int row, int col = 0) const;
    int tracksSize() const;
    void highlight(quint64 uid, enum HighlightState st);
    QModelIndex indexOf(quint64 uid) const;

    std::shared_ptr<Playlist::Playlist> playlist();
    void reload();

    void remove(const QList<QModelIndex> &items);

  private:
    void setTracks(const QVector<Track> &tracks);
    void allDataChanged();

    std::shared_ptr<Playlist::Playlist> _playlist;
    QVector<Track> tracks;
    quint64 highlight_uid;
    QStyle *style;
    enum HighlightState highlight_state;
    const ColumnsConfig &columns_config;
  };
}

#endif // PLAYLISTMODEL_H
