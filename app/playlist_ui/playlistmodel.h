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
    enum Roles {
      IsStreamRole = Qt::UserRole + 1,
      StationNameRole,
      StreamNowPlayingRole
    };
    explicit Model(QStyle *stl, const ColumnsConfig &col_cfg, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    void setPlaylist(std::shared_ptr<Playlist::Playlist> pl);

    Track itemAt(const QModelIndex &index) const;
    const Track &trackAt(int row) const; // row must be in [0, tracksSize())
    QModelIndex buildIndex(int row, int col = 0) const;
    int tracksSize() const;
    void highlight(quint64 uid, enum HighlightState st);
    void updateStreamMeta(quint64 uid, const StreamMetaData &meta);
    QModelIndex indexOf(quint64 uid) const;

    std::shared_ptr<Playlist::Playlist> playlist();
    virtual void reload();

    virtual void remove(const QList<QModelIndex> &items);

    virtual void appendToPlaylistAsync(const QList<QDir> &filepaths);
    void appendTracks(const QVector<Track> &tracks);
    virtual void insertTracksAsync(const QList<QDir> &filepaths, int atRow);

    virtual void sortBy(const QString &criteria);

  signals:
    void appendToPlaylistAsyncFinished(std::shared_ptr<Playlist::Playlist> pl);

  protected:
    virtual void removeTracksFromPlaylist(const QList<int> &indecies);

  private:
    void setTracks(const QVector<Track> &tracks);
    void allDataChanged();
    void applyStreamMeta();

    std::shared_ptr<Playlist::Playlist> _playlist;
    QVector<Track> tracks;
    quint64 highlight_uid = 0;
    quint64 stream_meta_uid = 0;
    StreamMetaData stream_meta;
    QStyle *style;
    enum HighlightState highlight_state = None;
    const ColumnsConfig &columns_config;
  };
}

#endif // PLAYLISTMODEL_H
