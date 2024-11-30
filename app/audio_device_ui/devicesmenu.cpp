#include "audio_device_ui/devicesmenu.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

#include <QMediaDevices>
#include <QAudioDevice>

namespace AudioDeviceUi {
  DevicesMenu::DevicesMenu(QWidget *parent, Config::Local &local_c) : QMenu(parent), local_conf(local_c) {
    auto devices = QMediaDevices::audioOutputs();
    auto action_group = new QActionGroup(this);
    action_group->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    for (auto &device : devices) {
      auto action = new QAction(action_group);
      QString text(device.description());
      if (device.isDefault()) {
        text.append(" [");
        text.append(tr("default"));
        text.append("]");
      }
      action->setText(text);
      action->setCheckable(true);
      action->setData(device.id());
      connect(action, &QAction::triggered, [=](bool checked) {
        if (checked) {
          on_selected(action->data().toByteArray());
        }
      });
      action_group->addAction(action);
      addAction(action);
    }
  }

  void DevicesMenu::on_selected(QByteArray deviceid) {
    auto devices = QMediaDevices::audioOutputs();
    for (auto &device : devices) {
      if (device.id() == deviceid) {
        qDebug() << "enable output to" << device.description();
      }
    }
  }
} // namespace AudioDeviceUi

#endif
