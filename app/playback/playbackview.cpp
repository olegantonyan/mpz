#include "playbackview.h"

namespace Playback {
  View::View(const Controls &c, QObject *parent) : QObject(parent), _controls(c) {
    connect(&_player, &QMediaPlayer::positionChanged, this, &View::on_positionChanged);
    connect(&_player, &QMediaPlayer::stateChanged, this, &View::on_stateChanged);
    connect(&_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &View::on_error);
    connect(_controls.stop, &QToolButton::clicked, this, &View::stop);
    connect(_controls.pause, &QToolButton::clicked, [=]() {
      if (_player.state() == QMediaPlayer::PausedState) {
        _player.play();
      } else  {
        _player.pause();
      }
    });
    connect(_controls.play, &QToolButton::clicked, [=]() {
      if (_player.state() == QMediaPlayer::PausedState) {
        _player.play();
      } else  {
        emit startRequested();
      }
    });
    connect(_controls.prev, &QToolButton::clicked, [=]() {
      next_after_stop = false;
      if (_current_track.isValid()) {
        emit prevRequested();
      }
    });
    connect(_controls.next, &QToolButton::clicked, [=]() {
      next_after_stop = false;
      if (_current_track.isValid()) {
        emit nextRequested();
      }
    });

    _controls.seekbar->installEventFilter(this);
    next_after_stop = true;
  }

  Controls View::controls() const {
    return _controls;
  }

  void View::play(const Track &track) {
    next_after_stop = false;
    _controls.seekbar->setMaximum(static_cast<int>(track.duration()));
    _player.setMedia(QUrl::fromLocalFile(track.path()));
    _current_track = track;
    _player.play();
  }

  void View::stop() {
    next_after_stop = false;
    _player.stop();
    emit stopped();
  }

  void View::on_seek(int position) {
    if (_player.state() != QMediaPlayer::PlayingState)  {
      return;
    }

    int total = _controls.seekbar->width();
    position = qMin(qMax(position, 0), total);

    double fraction = position / static_cast<double>(total);
    int seek_value = static_cast<int>(_controls.seekbar->maximum() * fraction);
    _controls.seekbar->setValue(seek_value);

    _player.setPosition(seek_value * 1000);
  }

  QString View::time_text(int pos) const {
    return QString("%1/%2").arg(Track::formattedTime(static_cast<quint32>(pos))).arg(_current_track.formattedDuration());
  }

  void View::on_positionChanged(quint64 pos) {
    int v = static_cast<int>(pos / 1000);
    _controls.time->setText(time_text(v));
    _controls.seekbar->setValue(v);
  }

  void View::on_stateChanged(QMediaPlayer::State state) {
    switch (state) {
      case QMediaPlayer::StoppedState:
        _controls.seekbar->setValue(0);
        _player.setMedia(nullptr);
        _controls.time->clear();
        _current_track = Track();
        if (next_after_stop) {
          emit nextRequested();
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
    emit nextRequested();
  }

  bool View::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      on_seek(me->pos().x());
    }
    return QObject::eventFilter(obj, event);
  }
}
