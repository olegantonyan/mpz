#ifndef OUTPUTDEVICENAME_H
#define OUTPUTDEVICENAME_H

#include <QByteArray>
#include <QString>

namespace AudioDeviceUi {
  void rememberOutputDevice(const QByteArray &id, const QString &description);

  // An empty id means "Default", named after whatever the system default is now.
  QString outputDeviceName(const QByteArray &id);
}

#endif // OUTPUTDEVICENAME_H
