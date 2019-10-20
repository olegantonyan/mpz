#include "playbackview.h"

namespace Playback {
  View::View(const Controls &c, QObject *parent) : QObject(parent), controls(c) {
    connect(&player, &QMediaPlayer::positionChanged, this, &View::on_positionChanged);
    connect(&player, &QMediaPlayer::stateChanged, this, &View::on_stateChanged);
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &View::on_error);
    connect(controls.stop, &QToolButton::clicked, &player, &QMediaPlayer::stop);
    connect(controls.pause, &QToolButton::clicked, &player, &QMediaPlayer::pause);
    connect(controls.play, &QToolButton::clicked, &player, &QMediaPlayer::play);
    connect(controls.prev, &QToolButton::clicked, [=]() {
      if (current_track().track.isValid()) {
        emit prev_requested(current_track());
      }
    });
    connect(controls.next, &QToolButton::clicked, [=]() {
      if (current_track().track.isValid()) {
        emit next_requested(current_track());
      }
    });

    auto interceptor = new MouseEventInterceptor(&Playback::View::on_seek, this);
    controls.seekbar->installEventFilter(interceptor);
  }

  TrackWrapper View::current_track() const {
    return _current_track;
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

  void View::on_positionChanged(quint64 pos) {
    controls.seekbar->setValue(static_cast<int>(pos / 1000));
  }

  void View::on_stateChanged(QMediaPlayer::State state) {
    switch (state) {
      case QMediaPlayer::StoppedState:
        controls.seekbar->setValue(0);
        _current_track = TrackWrapper();
        player.setMedia(nullptr);
        emit stopped();
        break;
      case QMediaPlayer::PlayingState:
        emit started(_current_track);
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
