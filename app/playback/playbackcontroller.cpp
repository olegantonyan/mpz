#include "playbackcontroller.h"
#include "streammetadata.h"

#include <QDebug>

namespace Playback {
Controller::Controller(const Controls &c, quint32 stream_buffer_size, QByteArray outdevid, ModusOperandi &modus, QObject *parent) :
  QObject(parent),
  _controls(c),
  _player(stream_buffer_size, outdevid),
  modus_operndi(modus)
#ifdef ENABLE_MPD_SUPPORT
  , _mpdplayer(stream_buffer_size, outdevid, modus.mpd_client)
#endif
{
    connect(&_player, &MediaPlayer::positionChanged, this, &Controller::on_positionChanged);
    connect(&_player, &MediaPlayer::stateChanged, this, &Controller::on_stateChanged);
    connect(&_player, &MediaPlayer::nextRequested, this, &Controller::nextRequested);
    connect(&_player, &MediaPlayer::prevRequested, this, &Controller::prevRequested);

    connect(&_player, &MediaPlayer::streamBufferfillChanged, [=](quint32 bytes, quint32 thresh) {
      Q_UNUSED(thresh)
      emit streamFill(_current_track, bytes);
    });
    connect(&_player, &MediaPlayer::streamMetaChanged, [=](const StreamMetaData &meta) {
      _current_track.setStreamMeta(meta);
      emit trackChanged(_current_track);
    });

#ifdef ENABLE_MPD_SUPPORT
    connect(&_mpdplayer, &MediaPlayer::positionChanged, this, &Controller::on_positionChanged);
    connect(&_mpdplayer, &MediaPlayer::stateChanged, this, &Controller::on_stateChanged);
    connect(&_mpdplayer, &Mpd::MediaPlayer::trackChanged, [=](auto path) {
      emit trackChangedQuery(path, _current_track.playlist_name());
    });
    connect(&modus_operndi.mpd_client, &MpdClient::Client::mixerChanged, [=]() {
      int volume = player().volume();
      emit volumeChanged(volume);
    });
#endif
    connect(&modus_operndi, &ModusOperandi::changed, this, &Controller::switchTo);

    connect(_controls.stop, &QToolButton::clicked, this, &Controller::stop);
    connect(_controls.pause, &QToolButton::clicked, this, &Controller::on_controlsPause);
    connect(_controls.play, &QToolButton::clicked, this, &Controller::on_controlsPlay);
    connect(_controls.prev, &QToolButton::clicked, this, &Controller::on_controlsPrev);
    connect(_controls.next, &QToolButton::clicked, this, &Controller::on_controlsNext);

    _controls.seekbar->installEventFilter(this);

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

  MediaPlayer &Controller::player() {
    switch (modus_operndi.get()) {
    case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
      return _mpdplayer;
#endif
    case ModusOperandi::MODUS_LOCALFS:
    default:
      return _player;
    }
  }

  Controls Controller::controls() const {
    return _controls;
  }

  int Controller::volume() {
    return player().volume();
  }

  bool Controller::isStopped() {
    return state() == Stopped;
  }

  Controller::State Controller::state() {
    switch (player().state()) {
      case MediaPlayer::StoppedState:
        return Stopped;
      case MediaPlayer::PlayingState:
        return Playing;
      case MediaPlayer::PausedState:
        return Paused;
    }
    return Stopped;
  }

  int Controller::position() {
    return static_cast<int>(player().position() / 1000);
  }

  const Track &Controller::currentTrack() const {
    return _current_track;
  }

  void Controller::play(const Track &track) {
#ifdef QT6_STREAM_HACKS
    if (track.isStream()) {
      player().stop();
      setCurrentTrack(track);
      player().setTrack(track);
    } else {
      player().setTrack(track);
      setCurrentTrack(track);
    }
#else
    player().setTrack(track);
    setCurrentTrack(track);
#endif
    player().play();
    if (track.isStream()) {
      _controls.seekbar->setMaximum(1);
    } else {
      _controls.seekbar->setMaximum(static_cast<int>(track.duration() / 1000));
    }
  }

  void Controller::stop() {
    player().stop();
  }

  void Controller::on_controlsPause() {
    player().pause();
  }

  void Controller::on_controlsNext() {
    if (_current_track.isValid()) {
      player().next();
    }
  }

  void Controller::on_controlsPrev() {
    if (_current_track.isValid()) {
      player().prev();
    }
  }

  void Controller::on_controlsPlay() {
    if (player().state() == MediaPlayer::PausedState) {
      player().play();
    } else  {
      emit startRequested();
    }
  }

  void Controller::setVolume(int value) {
    value = qMax(qMin(value, 100), 0);
    player().setVolume(value);
    emit volumeChanged(value);
  }

  void Controller::seek(int seconds) {
    player().setPosition(seconds * 1000);
  }

  void Controller::on_seek(int position) {
    if (player().state() != MediaPlayer::PlayingState)  {
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

    player().setPosition(seek_value * 1000);
    emit seeked(seek_value);
  }

  QString Controller::time_text(quint64 pos) const {
    return QString("%1/%2").arg(Track::formattedTime(static_cast<quint64>(pos))).arg(_current_track.formattedDuration());
  }

  void Controller::on_positionChanged(quint64 pos) {
    int v = static_cast<int>(pos / 1000);
    _controls.time->setText(time_text(pos));
    if (!_current_track.isStream()) {
      _controls.seekbar->setValue(v);
    }
    emit progress(_current_track, v);
  }

  void Controller::on_stateChanged(MediaPlayer::State state) {
    switch (state) {
      case MediaPlayer::StoppedState:
        _controls.seekbar->setValue(0);
        player().clearTrack();
        _controls.time->clear();
        setCurrentTrack(Track());
        emit stopped();
        break;
      case MediaPlayer::PlayingState:
        emit started(_current_track);
        break;
      case MediaPlayer::PausedState:
        emit paused(_current_track);
        break;
    }
  }

  void Controller::switchTo(ModusOperandi::ActiveMode new_mode) {
    switch (new_mode) {
    case ModusOperandi::MODUS_MPD:
      _player.stop();
      _player.clearTrack();
      break;
    case ModusOperandi::MODUS_LOCALFS:
    default:
#ifdef ENABLE_MPD_SUPPORT
      _mpdplayer.stop();
      _mpdplayer.clearTrack();
#endif
      break;
    }
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      on_seek(me->pos().x());
    }
    return QObject::eventFilter(obj, event);
  }

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  void Controller::setOutputDevice(QByteArray deviceid) {
    player().setOutputDevice(deviceid);
  }

  void Controller::setCurrentTrack(const Track &track) {
    _current_track = track;
  }

  void Controller::trackChangedQueryComplete(const Track &track) {
    setCurrentTrack(track);
    player().setTrack(track);
    emit started(track);
  }
#endif
}
