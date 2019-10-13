#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist.h"
#include "config/local.h"

#include <QObject>
#include <QTableView>
#include <memory>

namespace PlaylistUi {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(QTableView *v, Config::Local &conf, QObject *parent = nullptr);

  signals:

  public slots:
    void on_load(const std::shared_ptr<Playlist> pi);
    void on_unload();

  private:
    QTableView *view;
    Model *model;
    Config::Local &local_conf;
  };
}

#endif // PLAYLISTVIEW_H
