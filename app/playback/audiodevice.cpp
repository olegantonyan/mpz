#include "audiodevice.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

#include <QDebug>

namespace Playback {
  AudioDevice::AudioDevice(QObject *parent) : QObject(parent) {
    //qDebug() << audio_output.device().id() << audio_output.device().description();
    const auto devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : devices)
        qDebug() << "Device: " << device.id() << device.description();

    connect(&_devices, &QMediaDevices::audioOutputsChanged, [=]() {
      qDebug() << "change!";
      const auto devices = QMediaDevices::audioOutputs();
      for (const QAudioDevice &device : devices)
          qDebug() << "Device: " << device.id() << device.description();
    });
  }
} // namespace Playback

#endif
