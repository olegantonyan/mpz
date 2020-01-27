#ifndef PLAYLISTSCONTEXTMENU_H
#define PLAYLISTSCONTEXTMENU_H

#include "playlistsmodel.h"
#include "playlistsproxyfiltermodel.h"
#include "playlist.h"

#include <QObject>
#include <QPoint>
#include <QListView>
#include <QLineEdit>
#include <memory>
#include <QAction>

namespace PlaylistsUi {
  class PlaylistsContextMenu : public QObject {
    Q_OBJECT
  public:
    explicit PlaylistsContextMenu(Model *model, ProxyFilterModel *proxy, QListView *view, QLineEdit *seacrh, QObject *parent = nullptr);

  public slots:
    void show(const QPoint &pos);

  signals:
    void removed(const QModelIndex &index);

  private:
    Model *model;
    ProxyFilterModel *proxy;
    QListView *view;
    QLineEdit *search;
  };
}

#endif // PLAYLISTSCONTEXTMENU_H
