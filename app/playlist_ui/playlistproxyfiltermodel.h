#ifndef PLAYLISTPROXYFILTERMODEL_H
#define PLAYLISTPROXYFILTERMODEL_H

#include "playlistmodel.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpd/playlistmodel.h"
#endif
#include "modusoperandi.h"

#include <QObject>
#include <QSortFilterProxyModel>

namespace PlaylistUi {
  class ProxyFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit ProxyFilterModel(QStyle *stl, const ColumnsConfig &col_cfg, ModusOperandi &modus, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Model *activeModel() const;

  signals:
    void appendToPlaylistAsyncFinished(std::shared_ptr<Playlist::Playlist> pl);

  public slots:
    void filter(const QString &term);

  private slots:
    void switchTo(ModusOperandi::ActiveMode new_mode);

  protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;

  private:
    Model *localfs;
#ifdef ENABLE_MPD_SUPPORT
    Mpd::Model *mpd;
#endif
    ModusOperandi &modus_operandi;
    QString filter_term;
  };
}

#endif // PLAYLISTPROXYFILTERMODEL_H
