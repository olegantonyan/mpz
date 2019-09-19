#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"

#include <QObject>
#include <QTableView>

namespace Playlist {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(QTableView *v, QObject *parent = nullptr);

  signals:

  public slots:

  private:
    QTableView *view;
    Playlist::Model *model;
  };
}

#endif // PLAYLISTVIEW_H
