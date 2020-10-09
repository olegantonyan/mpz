#ifndef PLAYLISTCONTEXTMENU_H
#define PLAYLISTCONTEXTMENU_H

#include "playlistmodel.h"
#include "playlistproxyfiltermodel.h"
#include "playlist/playlist.h"

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
    explicit PlaylistContextMenu(Model *model, ProxyFilterModel *proxy, QTableView *view, QLineEdit *seacrh, QObject *parent = nullptr);

  public slots:
    void show(const QPoint &pos);
    void on_remove();

  signals:
    void playlistChanged(const std::shared_ptr<Playlist::Playlist> pl);

  private:
    Model *model;
    ProxyFilterModel *proxy;
    QTableView *view;
    QLineEdit *search;

    QAction remove;
    QAction clear_filter;
    QAction show_in_filemanager;
    QAction copy_name;
    QAction info;

  private slots:
    void on_clearFilter();
    void on_copyName();
    void on_showInFilemanager();
    void on_trackInfo();
  };
}

#endif // PLAYLISTCONTEXTMENU_H
