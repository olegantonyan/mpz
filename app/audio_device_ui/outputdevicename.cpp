#include "audio_device_ui/outputdevicename.h"

#include <QAudioDevice>
#include <QCoreApplication>
#include <QHash>
#include <QMediaDevices>

namespace AudioDeviceUi {
  namespace {
    // Remembers descriptions of devices seen plugged in, so the configured but
    // currently disconnected device can still be shown by name.
    QHash<QByteArray, QString> &descriptionCache() {
      static QHash<QByteArray, QString> cache;
      return cache;
    }
  }

  void rememberOutputDevice(const QByteArray &id, const QString &description) {
    descriptionCache().insert(id, description);
  }

  QString outputDeviceName(const QByteArray &id) {
    if (id.isEmpty()) {
      const QString description = QMediaDevices::defaultAudioOutput().description();
      const QString name = QCoreApplication::translate("AudioDeviceUi", "Default");
      return description.isEmpty() ? name : name + " [" + description + "]";
    }

    const auto devices = QMediaDevices::audioOutputs();
    for (const auto &device : devices) {
      if (device.id() == id) {
        return device.description();
      }
    }
    return descriptionCache().value(id, QString::fromLatin1(id));
  }
}
