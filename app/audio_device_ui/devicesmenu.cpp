#include "audio_device_ui/devicesmenu.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

#include <QMediaDevices>
#include <QAudioDevice>

namespace AudioDeviceUi {
  DevicesMenu::DevicesMenu(QWidget *parent, Config::Local &local_c) : QMenu(parent), local_conf(local_c) {
    auto devices = QMediaDevices::audioOutputs();
    for (auto &device : devices) {
      auto action = new QAction(this);
      action->setText(device.description());
      action->setCheckable(true);
      action->setProperty("device_id", device.id());
      connect(action, &QAction::triggered, [=](bool checked) {
        qDebug() << "triggered" << checked << action->property("device_id");
      });

      addAction(action);
    }
  }
} // namespace AudioDeviceUi

#endif
