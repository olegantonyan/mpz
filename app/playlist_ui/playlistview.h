#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist.h"

#include <QObject>
#include <QTableView>
#include <memory>
#include <QEvent>

namespace PlaylistUi {
  class ResizeEventInterceptor : public QObject {
    Q_OBJECT

  protected:
    bool eventFilter(QObject *obj, QEvent *event);
  };

  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(QTableView *v, QObject *parent = nullptr);

  signals:

  public slots:
    void on_load(const std::shared_ptr<Playlist> pi);
    void on_unload();

  private:
    QTableView *view;
    Model *model;
    ResizeEventInterceptor interceptor;
  };
}

#endif // PLAYLISTVIEW_H
