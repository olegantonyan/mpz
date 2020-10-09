#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist/playlist.h"
#include "track.h"
#include "config/local.h"
#include "playlistproxyfiltermodel.h"
#include "playlist_ui/playlistcontextmenu.h"

#include <QObject>
#include <QTableView>
#include <memory>
#include <QEvent>
#include <QLineEdit>
#include <QHash>

namespace PlaylistUi {
  class Controller : public QObject {
    Q_OBJECT
  public:
    explicit Controller(QTableView *v, QLineEdit *search, Config::Local &local_cfg, QObject *parent = nullptr);

  signals:
    void activated(const Track &track);
    void selected(const Track &track);
    void changed(const std::shared_ptr<Playlist::Playlist> pl);
    void durationOfSelectedChanged(quint32 total_duration);

  public slots:
    void on_load(const std::shared_ptr<Playlist::Playlist> pi);
    void on_unload();
    void on_stop();
    void on_start(const Track &t);
    void on_pause(const Track &t);
    void on_scrollTo(const Track &track);
    void on_appendToPlaylist(const QDir &filepath);

  private slots:
    void on_appendAsyncFinished(Playlist::Playlist *pl);
    void on_search(const QString &term);
    void on_currentSelectionChanged(const QModelIndex &index, const QModelIndex &prev);
    void on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

  private:
    QTableView *view;
    QLineEdit *search;
    Model *model;
    Config::Local &local_conf;
    bool restore_scroll_once;
    QHash<quint64,int> scroll_positions;
    ProxyFilterModel *proxy;
    PlaylistContextMenu *context_menu;

    void eventFilterTableView(QEvent *event);
    void eventFilterViewport(QEvent *event);
    
  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // PLAYLISTVIEW_H
