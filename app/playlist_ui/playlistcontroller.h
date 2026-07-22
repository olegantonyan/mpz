#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist/playlist.h"
#include "track.h"
#include "config/local.h"
#include "config/global.h"
#include "playlistproxyfiltermodel.h"
#include "playlist_ui/playlistcontextmenu.h"
#include "busyspinner.h"
#include "playlist_ui/columnsconfig.h"

#include <QObject>
#include <QTableView>
#include <memory>
#include <QEvent>
#include <QLineEdit>
#include <QHash>

class QDropEvent;

namespace PlaylistUi {
  class Controller : public QObject {
    Q_OBJECT
  public:
    explicit Controller(QTableView *v, QLineEdit *search, BusySpinner *_spinner, Config::Local &local_cfg, Config::Global &global_cfg, ModusOperandi &modus, QObject *parent = nullptr);

  signals:
    void activated(const Track &track);
    void selected(const Track &track);
    void changed(const std::shared_ptr<Playlist::Playlist> pl);
    void durationOfSelectedChanged(quint32 total_duration);
    void createPlaylistRequested(const QList<QDir> &filepaths, const QString &libraryDir);

  public slots:
    void on_load(const std::shared_ptr<Playlist::Playlist> pi);
    void on_unload();
    void on_stop();
    void on_start(const Track &t);
    void on_pause(const Track &t);
    void on_scrollTo(const Track &track);
    void on_trackMetaChanged(const Track &t);
    void on_appendToPlaylist(const QList<QDir> &filepaths);
    void on_appendTracks(const QVector<Track> &tracks);
    void sortBy(const QString &criteria);

  private slots:
    void on_appendAsyncFinished(std::shared_ptr<Playlist::Playlist> pl);
    void on_search(const QString &term);
    void on_currentSelectionChanged(const QModelIndex &index, const QModelIndex &prev);
    void on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void on_tracksChanged(const std::shared_ptr<Playlist::Playlist> pl, const QList<quint64> &uids);

  private:
    QTableView *view;
    QLineEdit *search;
    BusySpinner *spinner;
    Config::Local &local_conf;
    Config::Global &global_conf;
    ModusOperandi &modus_operandi;
    bool restore_scroll_once;
    bool persist_pending = false;
    QHash<quint64,int> scroll_positions;
    ProxyFilterModel *proxy;
    PlaylistContextMenu *context_menu;
    ColumnsConfig columns_config;
    quint64 live_stream_uid = 0;

    void updateStreamSpans();
    void eventFilterTableView(QEvent *event);
    void eventFilterViewport(QEvent *event);
    bool handleExternalDnd(QEvent *event);
    void onExternalDrop(QDropEvent *event);

    void loadColumnsConfig();

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // PLAYLISTVIEW_H
