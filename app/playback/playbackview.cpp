#include "playbackview.h"

namespace Playback {
  View::View(const Controls &c, QObject *parent) : QObject(parent), controls(c) {
    connect(&player, &QMediaPlayer::positionChanged, this, &View::on_positionChanged);
    connect(&player, &QMediaPlayer::stateChanged, this, &View::on_stateChanged);
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &View::on_error);
    connect(controls.stop, &QToolButton::clicked, this, &View::stop);
    connect(controls.pause, &QToolButton::clicked, &player, &QMediaPlayer::pause);
    connect(controls.play, &QToolButton::clicked, [=]() {
      if (player.state() == QMediaPlayer::PausedState) {
        player.play();
      } else  {
        emit start_requested();
      }
    });
    connect(controls.prev, &QToolButton::clicked, [=]() {
      next_after_stop = false;
      if (_current_track.track.isValid()) {
        emit prev_requested();
      }
    });
    connect(controls.next, &QToolButton::clicked, [=]() {
      next_after_stop = false;
      if (_current_track.track.isValid()) {
        emit next_requested();
      }
    });

    auto interceptor = new EventInterceptor(&Playback::View::on_event, this);
    controls.seekbar->installEventFilter(interceptor);
    next_after_stop = true;
  }

  void View::play(const TrackWrapper &track) {
    controls.seekbar->setMaximum(static_cast<int>(track.track.duration()));
    player.setMedia(QUrl::fromLocalFile(track.track.path()));
    _current_track = track;
    player.play();
  }

  void View::stop() {
    controls.seekbar->setValue(0);
    player.stop();
    next_after_stop = false;
    emit stopped();
  }

  void View::on_seek(int position) {
    if (player.state() != QMediaPlayer::PlayingState)  {
      return;
    }

    int total = controls.seekbar->width();
    position = qMin(qMax(position, 0), total);

    double fraction = position / static_cast<double>(total);
    int seek_value = static_cast<int>(controls.seekbar->maximum() * fraction);
    controls.seekbar->setValue(seek_value);

    player.setPosition(seek_value * 1000);
  }

  void View::on_event(QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      on_seek(me->pos().x());
    }
  }

  QString View::time_text(int pos) const {
    return QString("%1/%2").arg(Track::formattedTime(static_cast<quint32>(pos))).arg(_current_track.track.formattedDuration());
  }

  void View::on_positionChanged(quint64 pos) {
    int v = static_cast<int>(pos / 1000);
    controls.time->setText(time_text(v));
    controls.seekbar->setValue(v);
  }

  void View::on_stateChanged(QMediaPlayer::State state) {
    switch (state) {
      case QMediaPlayer::StoppedState:
        controls.seekbar->setValue(0);
        player.setMedia(nullptr);
        controls.time->clear();
        _current_track = TrackWrapper();
        if (next_after_stop) {
          emit next_requested();
        }
        break;
      case QMediaPlayer::PlayingState:
        emit started(_current_track);
        next_after_stop = true;
        break;
      case QMediaPlayer::PausedState:
        emit paused(_current_track);
        break;
    }
  }

  void View::on_error(QMediaPlayer::Error error) {
    qDebug() << "error" << error;
  }
}
