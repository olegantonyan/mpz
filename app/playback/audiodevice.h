#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <QObject>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioSink>

namespace Playback {
  class AudioDevice : public QObject {
    Q_OBJECT
  public:
    explicit AudioDevice(QObject *parent = nullptr);

  signals:
    void changed(QAudioDevice device);

  private:
    QMediaDevices _devices;

  };
} // namespace Playback

#endif

#endif // AUDIODEVICE_H
