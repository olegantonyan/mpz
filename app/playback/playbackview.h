#ifndef PLAYBACKVIEW_H
#define PLAYBACKVIEW_H

#include "controls.h"
#include "trackwrapper.h"

#include <QObject>
#include <QMediaPlayer>
#include <memory>
#include <QEvent>
#include <QMouseEvent>

namespace Playback {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(const Playback::Controls &c, QObject *parent = nullptr);

    TrackWrapper current_track() const;

  signals:
    void started(const TrackWrapper &track);
    void stopped();
    void paused(const TrackWrapper &track);
    void prev_requested(const TrackWrapper &current);
    void next_requested(const TrackWrapper &current);

  public slots:
    void play(const TrackWrapper &track);
    void stop();

  private:
    Playback::Controls controls;
    QMediaPlayer player;
    TrackWrapper _current_track;

    void on_seek(int position);

    QString time_text(int pos) const;

  private slots:
    void on_positionChanged(quint64 pos);
    void on_stateChanged(QMediaPlayer::State state);
    void on_error(QMediaPlayer::Error error);
  };

  //
  class MouseEventInterceptor : public QObject {
    Q_OBJECT
  public:
    explicit MouseEventInterceptor(void (Playback::View::*cb)(int), Playback::View *cbobj) :
      QObject(cbobj), callback_object(cbobj), callback(cb) {
    }

  protected:
    bool eventFilter(QObject *obj, QEvent *event) {
      if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
        (callback_object->*callback)(me->pos().x());
      }
      return QObject::eventFilter(obj, event);
    }
  private:
    Playback::View *callback_object;
    void (Playback::View::*callback)(int);
  };
}
#endif // PLAYBACKVIEW_H