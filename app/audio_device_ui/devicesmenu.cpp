#include "audio_device_ui/devicesmenu.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

#include <QMediaDevices>
#include <QAudioDevice>

namespace AudioDeviceUi {
  DevicesMenu::DevicesMenu(QWidget *parent, Config::Local &local_c) : QMenu(parent), local_conf(local_c) {
    auto devices = QMediaDevices::audioOutputs();
    auto action_group = new QActionGroup(this);
    auto action_default = new QAction(action_group);
    QString default_text(tr("Default"));
    action_default->setText(default_text);
    action_default->setCheckable(true);
    connect(action_default, &QAction::triggered, [=](bool checked) {
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
      QString text(device.description());
      if (device.isDefault()) {
        default_text.append(" [");
        default_text.append(device.description());
        default_text.append("]");
        action_default->setText(default_text);
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

      if (!isDefaultOutput() && currentOutput() == device.id()) {
        action->setChecked(true);
        device_exists = true;
      }

      devices_id_description_cache.insert(device.id(), device.description());
    }

    if (!device_exists) {
      auto action = new QAction(action_group);
      auto text = devices_id_description_cache.value(
            currentOutput(),
            QString::fromStdString(currentOutput().toStdString())
            );
      action->setText(text);
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

#endif
