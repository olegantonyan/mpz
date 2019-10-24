#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist.h"
#include "trackwrapper.h"

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
    void activated(const TrackWrapper &track);

  public slots:
    void on_load(const std::shared_ptr<Playlist> pi, int playlist_index);
    void on_unload();
    void on_prev_requested(const TrackWrapper &track);
    void on_next_requested(const TrackWrapper &track);
    void on_start_requested();
    void on_started(const TrackWrapper &track);
    void on_stopped();

  private:
    QTableView *view;
    Model *model;

    void on_resize();
  };


  //
  class ResizeEventInterceptor : public QObject {
    Q_OBJECT
  public:
    explicit ResizeEventInterceptor(void (PlaylistUi::View::*cb)(), PlaylistUi::View *cbobj) :
      QObject(cbobj), callback_object(cbobj), callback(cb) {
    }

  protected:
    bool eventFilter(QObject *obj, QEvent *event) {
      if (event->type() == QEvent::Resize) {
        (callback_object->*callback)();
      }
      return QObject::eventFilter(obj, event);
    }

  private:
    PlaylistUi::View *callback_object;
    void (PlaylistUi::View::*callback)();
  };
}

#endif // PLAYLISTVIEW_H
