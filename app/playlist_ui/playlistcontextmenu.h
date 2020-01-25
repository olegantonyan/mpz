#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include "playlistmodel.h"
#include "playlistproxyfiltermodel.h"
#include "playlist.h"

#include <QObject>
#include <QPoint>
#include <QTableView>
#include <QLineEdit>
#include <memory>

namespace PlaylistUi {
  class PlaylistContextMenu : public QObject {
    Q_OBJECT
  public:
    explicit PlaylistContextMenu(Model *model, ProxyFilterModel *proxy, QTableView *view, QLineEdit *s, QObject *parent = nullptr);

  public slots:
    void show(const QPoint &pos);

  signals:
    void playlistChanged(const std::shared_ptr<Playlist> pl);

  private:
    Model *model;
    ProxyFilterModel *proxy;
    QTableView *view;
    QLineEdit *search;
  };
}

#endif // CONTEXTMENU_H
