#ifndef PLAYLISTCONTEXTMENU_H
#define PLAYLISTCONTEXTMENU_H

#include "playlistmodel.h"
#include "playlistproxyfiltermodel.h"
#include "playlist/playlist.h"
#include "config/global.h"

#include <QObject>
#include <QPoint>
#include <QTableView>
#include <QLineEdit>
#include <memory>
#include <QAction>

namespace PlaylistUi {
  class PlaylistContextMenu : public QObject {
    Q_OBJECT
  public:
    explicit PlaylistContextMenu(ProxyFilterModel *proxy, QTableView *view, QLineEdit *seacrh, Config::Global &global, QObject *parent = nullptr);

  public slots:
    void show(const QPoint &pos);
    void on_remove();
    void on_trackInfo();

  signals:
    void playlistChanged(const std::shared_ptr<Playlist::Playlist> pl);
    void tracksChanged(const std::shared_ptr<Playlist::Playlist> pl, const QList<quint64> &uids);

  private:
    ProxyFilterModel *proxy;
    QTableView *view;
    QLineEdit *search;
    Config::Global &global_conf;

    QAction remove;
    QAction clear_filter;
    QAction show_in_filemanager;
    QAction copy_name;
    QAction info;
    QAction edit_tags;

  private slots:
    void on_clearFilter();
    void on_copyName();
    void on_showInFilemanager();
    void on_editTags();
  };
}

#endif // PLAYLISTCONTEXTMENU_H
