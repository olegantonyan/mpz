#include "audio_device_ui/devicesmenu.h"
#include "audio_device_ui/outputdevicename.h"

#include <QMediaDevices>
#include <QAudioDevice>

namespace AudioDeviceUi {
  DevicesMenu::DevicesMenu(QWidget *parent, Config::Local &local_c) : QMenu(parent), local_conf(local_c) {
    // Rebuild on every open so hot-plugged devices and the current selection
    // stay in sync (matters for a persistent submenu; harmless for the
    // recreated-per-click toolbar popup).
    connect(this, &QMenu::aboutToShow, this, &DevicesMenu::populate);
    populate();
  }

  void DevicesMenu::populate() {
    clear();
    delete action_group;

    auto devices = QMediaDevices::audioOutputs();
    action_group = new QActionGroup(this);
    auto action_default = new QAction(action_group);
    action_default->setText(outputDeviceName(QByteArray()));
    action_default->setCheckable(true);
    connect(action_default, &QAction::triggered, this, [=](bool checked) {
      if (checked) {
        on_selected(QByteArray());
      }
    });
    action_group->addAction(action_default);
    addAction(action_default);
    addSeparator();

    bool device_exists = false;
    if (isDefaultOutput()) {
      action_default->setChecked(true);
      device_exists = true;
    }

    action_group->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    for (auto &device : devices) {
      auto action = new QAction(action_group);
      action->setText(device.description());
      action->setCheckable(true);
      action->setData(device.id());
      connect(action, &QAction::triggered, this, [=](bool checked) {
        if (checked) {
          on_selected(action->data().toByteArray());
        }
      });
      action_group->addAction(action);
      addAction(action);

      if (!isDefaultOutput() && currentOutput() == device.id()) {
        action->setChecked(true);
        device_exists = true;
      }

      rememberOutputDevice(device.id(), device.description());
    }

    if (!device_exists) {
      auto action = new QAction(action_group);
      action->setText(outputDeviceName(currentOutput()));
      action->setDisabled(true);
      action->setCheckable(true);
      action->setChecked(true);
      action_group->addAction(action);
      addAction(action);
    }
  }

  void DevicesMenu::on_selected(QByteArray deviceid) {
    if (deviceid.isEmpty()) {
      saveDefaultOutput();
      emit outputDeviceChanged(deviceid);
    } else {
      auto devices = QMediaDevices::audioOutputs();
      for (auto &device : devices) {
        if (device.id() == deviceid) {
          saveOutput(deviceid);
          emit outputDeviceChanged(deviceid);
          break;
        }
      }
    }
  }

  bool DevicesMenu::saveDefaultOutput() {
    return saveOutput(QByteArray());
  }

  bool DevicesMenu::saveOutput(QByteArray id) {
    return local_conf.saveOutputDeviceId(id);
  }

  bool DevicesMenu::isDefaultOutput() const {
    return currentOutput().isEmpty();
  }

  QByteArray DevicesMenu::currentOutput() const {
    return local_conf.outputDeviceId();
  }
} // namespace AudioDeviceUi
