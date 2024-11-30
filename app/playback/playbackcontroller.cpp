#include "playbackcontroller.h"
#include "streammetadata.h"

#include <QDebug>

namespace Playback {
  Controller::Controller(const Controls &c, quint32 stream_buffer_size, QObject *parent) : QObject(parent), _controls(c), _player(stream_buffer_size) {
    connect(&_player, &MediaPlayer::positionChanged, this, &Controller::on_positionChanged);
    connect(&_player, &MediaPlayer::stateChanged, this, &Controller::on_stateChanged);
    //connect(&_player, &MediaPlayer::error, this, &Controller::on_error);
    connect(_controls.stop, &QToolButton::clicked, this, &Controller::stop);
    connect(_controls.pause, &QToolButton::clicked, [=]() {
      if (_player.state() == MediaPlayer::PausedState) {
        _player.play();
      } else  {
        _player.pause();
      }
    });
    connect(_controls.play, &QToolButton::clicked, [=]() {
      if (_player.state() == MediaPlayer::PausedState) {
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
    connect(&_player, &MediaPlayer::streamBufferfillChanged, [=](quint32 bytes, quint32 thresh) {
      //double percents = static_cast<double>(bytes) / static_cast<double>(thresh) * 100.0;
      Q_UNUSED(thresh)
      emit streamFill(_current_track, bytes);
    });
    connect(&_player, &MediaPlayer::streamMetaChanged, [=](const StreamMetaData &meta) {
      _current_track.setStreamMeta(meta);
      emit trackChanged(_current_track);
    });

    _controls.seekbar->installEventFilter(this);
    next_after_stop = true;

    setup_monotonic_timer();
  }

  void Playback::Controller::setup_monotonic_timer() {
    monotonic_timer.setSingleShot(false);
    monotonic_timer.setInterval(1000);
    monotonic_timer.start();
    monotonic_timer.setTimerType(Qt::PreciseTimer);
    connect(&monotonic_timer, &QTimer::timeout, [=]() {
      if (state() == Controller::Playing) {
        emit monotonicPlaybackTimerIncrement(1);
      }
    });
  }

  Controls Controller::controls() const {
    return _controls;
  }

  int Controller::volume() const {
    return _player.volume();
  }

  bool Controller::isStopped() const {
    return state() == Stopped;
  }

  Controller::State Controller::state() const {
    switch (_player.state()) {
      case MediaPlayer::StoppedState:
        return Stopped;
      case MediaPlayer::PlayingState:
        return Playing;
      case MediaPlayer::PausedState:
        return Paused;
    }
    return Stopped;
  }

  int Controller::position() const {
    return static_cast<int>(_player.position() / 1000);
  }

  const Track &Controller::currentTrack() const {
    return _current_track;
  }

  void Controller::play(const Track &track) {
    next_after_stop = false;
    _player.setTrack(track);
    _current_track = track;
    _player.play();
    if (track.isStream()) {
      _controls.seekbar->setMaximum(1);
    } else {
      _controls.seekbar->setMaximum(static_cast<int>(track.duration()));
    }
  }

  void Controller::stop() {
    next_after_stop = false;
    _player.stop();
    emit stopped();
  }

  void Controller::setVolume(int value) {
    value = qMax(qMin(value, 100), 0);
    _player.setVolume(value);
    emit volumeChanged(value);
  }

  void Controller::seek(int seconds) {
    _player.setPosition(seconds * 1000);
  }

  void Controller::on_seek(int position) {
    if (_player.state() != MediaPlayer::PlayingState)  {
      return;
    }
    if (_current_track.isStream()) {
      return;
    }

    int total = _controls.seekbar->width();
    position = qMin(qMax(position, 0), total);

    double fraction = position / static_cast<double>(total);
    int seek_value = static_cast<int>(_controls.seekbar->maximum() * fraction);
    _controls.seekbar->setValue(seek_value);

    _player.setPosition(seek_value * 1000);
    emit seeked(seek_value);
  }

  QString Controller::time_text(int pos) const {
    return QString("%1/%2").arg(Track::formattedTime(static_cast<quint32>(pos))).arg(_current_track.formattedDuration());
  }

  void Controller::on_positionChanged(quint64 pos) {
    int v = static_cast<int>(pos / 1000);
    _controls.time->setText(time_text(v));
    if (!_current_track.isStream()) {
      _controls.seekbar->setValue(v);
    }
    emit progress(_current_track, v);
  }

  void Controller::on_stateChanged(MediaPlayer::State state) {
    switch (state) {
      case MediaPlayer::StoppedState:
        _controls.seekbar->setValue(0);
        _player.clearTrack();
        _controls.time->clear();
        _current_track = Track();
        if (next_after_stop) {
          emit nextRequested();
        }
        break;
      case MediaPlayer::PlayingState:
        emit started(_current_track);
        next_after_stop = true;
        break;
      case MediaPlayer::PausedState:
        emit paused(_current_track);
        break;
    }
  }

  void Controller::on_error(const QString &message) {
    qWarning() << "playback error" << message;
    emit nextRequested();
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      on_seek(me->pos().x());
    }
    return QObject::eventFilter(obj, event);
  }
}
