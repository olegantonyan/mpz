#include "audio_device_ui/outputdevicebutton.h"
#include "audio_device_ui/devicesmenu.h"
#include "audio_device_ui/outputdevicename.h"
#include "icons.h"

#include <QEvent>
#include <QActionGroup>
#include <QFontMetrics>
#include <QMenu>
#include <QMediaDevices>
#include <QPoint>

namespace AudioDeviceUi {
  namespace {
    const int name_max_chars = 20;
  }

  OutputDeviceButton::OutputDeviceButton(QToolButton *btn, Config::Local &local_c) : QObject(btn), button(btn), local_conf(local_c) {
    connect(button, &QToolButton::clicked, this, &OutputDeviceButton::on_buttonClicked);

    button->setIcon(Icons::get(Icons::Icon::Headphones));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    button->installEventFilter(this);
    updateSize();

    auto *media_devices = new QMediaDevices(this);
    connect(media_devices, &QMediaDevices::audioOutputsChanged, this, &OutputDeviceButton::refresh);
  }

  bool OutputDeviceButton::eventFilter(QObject *obj, QEvent *event) {
    if (obj == button && (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)) {
      // The button's size hint still reflects the old font/style while the event is being filtered.
      QMetaObject::invokeMethod(this, [this] { updateSize(); }, Qt::QueuedConnection);
    }
    return QObject::eventFilter(obj, event);
  }

  // Sizes the button from an ASCII sample, so device names carrying fallback-font glyphs (e.g. "MKⅢ") cannot inflate it out of line with the rest of the row.
  void OutputDeviceButton::updateSize() {
    const QString sample(name_max_chars, u'A');
    button->setText(sample);
    button->setFixedSize(button->sizeHint());
    text_width = button->fontMetrics().horizontalAdvance(sample);
    refresh();
  }

  void OutputDeviceButton::refresh() {
#ifdef ENABLE_MPZD_SUPPORT
    const QString remote_id = remote ? remote->status().output : QString();
    const QString name = remote ? remote_names.value(remote_id, remote_id) : outputDeviceName(local_conf.outputDeviceId());
#else
    const QString name = outputDeviceName(local_conf.outputDeviceId());
#endif
    button->setText(button->fontMetrics().elidedText(name, Qt::ElideRight, text_width));
    button->setToolTip(name);
  }

  QPoint OutputDeviceButton::menuPos(int menu_width) const {
    return button->mapToGlobal(QPoint(button->width() - menu_width, button->height()));
  }

  void OutputDeviceButton::on_buttonClicked() {
#ifdef ENABLE_MPZD_SUPPORT
    if (remote) {
      showRemoteMenu();
      return;
    }
#endif
    DevicesMenu menu(button, local_conf);
    connect(&menu, &DevicesMenu::outputDeviceChanged, this, &OutputDeviceButton::outputDeviceChanged);
    menu.exec(menuPos(menu.sizeHint().width()));
    refresh();
  }

#ifdef ENABLE_MPZD_SUPPORT
  void OutputDeviceButton::setRemote(MpzdClient::Client *client) {
    if (remote == client) {
      return;
    }
    if (remote) {
      disconnect(remote, nullptr, this, nullptr);
    }
    remote = client;
    remote_names.clear();
    if (remote) {
      connect(remote, &MpzdClient::Client::statusChanged, this, &OutputDeviceButton::refresh);
      connect(remote, &MpzdClient::Client::connected, this, &OutputDeviceButton::loadRemoteNames);
      if (remote->isConnected()) {
        loadRemoteNames();
      }
    }
    refresh();
  }

  void OutputDeviceButton::loadRemoteNames() {
    remote->request({"outputs"}, this, [this](const MpzdClient::Reply &reply) {
      remote_names.clear();
      for (const auto &rec : reply.records("output")) {
        remote_names.insert(rec.value("output"), rec.value("desc"));
      }
      refresh();
    });
  }

  void OutputDeviceButton::showRemoteMenu() {
    remote->request({"outputs"}, this, [this](const MpzdClient::Reply &reply) {
      if (!reply.ok || !remote) {
        return;
      }
      auto menu = new QMenu(button);
      menu->setAttribute(Qt::WA_DeleteOnClose);
      auto group = new QActionGroup(menu);
      for (const auto &rec : reply.records("output")) {
        const QString id = rec.value("output");
        remote_names.insert(id, rec.value("desc"));
        auto action = menu->addAction(rec.value("desc"));
        action->setCheckable(true);
        action->setChecked(rec.value("active") == "1");
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, id]() {
          if (remote) {
            remote->request({"output", id});
          }
        });
      }
      menu->popup(menuPos(menu->sizeHint().width()));
    });
  }
#endif
} // namespace AudioDeviceUi
