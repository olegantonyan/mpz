#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist.h"

#include <QObject>
#include <QTableView>

namespace PlaylistUi {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(QTableView *v, QObject *parent = nullptr);

  signals:

  public slots:
    void on_load(const Playlist &pi);

  private:
    QTableView *view;
    Model *model;
  };
}

#endif // PLAYLISTVIEW_H
