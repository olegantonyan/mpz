#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "streammetadata.h"
#include "track.h"

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QList>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  // Defined while the QtMultimedia backend is in use to enable workarounds for
  // Qt6 QMediaPlayer streaming quirks (see Playback::Qtmm::MediaPlayer and
  // Playback::Controller::play). Retired once the native engine is the only
  // stream backend.
  #define QT6_STREAM_HACKS
#endif

namespace Playback {
  // Abstract audio-backend interface. Concrete backends:
  //   - Playback::Qtmm::MediaPlayer   (QtMultimedia, fallback)
  //   - Playback::Native::MediaPlayer (FFmpeg + miniaudio, default)
  //   - Playback::Mpd::MediaPlayer    (MPD daemon)
  class MediaPlayer : public QObject {
    Q_OBJECT
  public:
    enum State {
      StoppedState,
      PlayingState,
      PausedState
    };
    Q_ENUM(State)

    // Backend-neutral audio-output device descriptor. `id` is an opaque token
    // understood by the backend that produced it (passed back to
    // setOutputDevice); an empty id means the system default.
    struct AudioDevice {
      QByteArray id;
      QString description;
      bool is_default = false;
    };

    explicit MediaPlayer(QObject *parent = nullptr);
    ~MediaPlayer() override = default;

    virtual State state() = 0;
    virtual int volume() = 0;
    virtual qint64 position() = 0;

    // Output devices the active backend can play to. Default: none (e.g. MPD,
    // or QtMultimedia on Qt5). The device UI enumerates through this so the ids
    // always match the backend that will receive setOutputDevice().
    virtual QList<AudioDevice> audioOutputs() { return {}; }

  signals:
    void positionChanged(qint64 position);
    void stateChanged(MediaPlayer::State newState);
    void error(const QString &message);
    void streamBufferfillChanged(quint32 current, quint32 total);
    void streamMetaChanged(const StreamMetaData& meta);
    void prevRequested();
    void nextRequested();
    void audioOutputsChanged();   // device list changed (hot-plug)

  public slots:
    virtual void pause() = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void next();
    virtual void prev();
    virtual void setPosition(qint64 position) = 0;
    virtual void setVolume(int volume) = 0;
    virtual void setTrack(const Track &track) = 0;
    virtual void clearTrack() = 0;
    // Output-device selection. Backends without device control (MPD) use the
    // no-op default. Available on every Qt version now that device handling no
    // longer depends on QMediaDevices.
    virtual void setOutputDevice(QByteArray deviceid);
  };
}

#endif // MEDIAPLAYER_H
