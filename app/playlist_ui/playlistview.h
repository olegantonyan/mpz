#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist.h"
#include "track.h"
#include "eventinteceptor.h"

#include <QObject>
#include <QTableView>
#include <memory>
#include <QEvent>

namespace PlaylistUi {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(QTableView *v, QObject *parent = nullptr);

  signals:
    void activated(Track track, int index);
    void selected(Track track, int index);

  public slots:
    void on_load(const std::shared_ptr<Playlist> pi);
    void on_unload();
    void highlight(int row);

  private:
    QTableView *view;
    Model *model;

    void on_event(QEvent *event);
  };
}

#endif // PLAYLISTVIEW_H
