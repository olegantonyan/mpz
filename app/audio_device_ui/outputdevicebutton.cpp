#include "audio_device_ui/outputdevicebutton.h"
#include "audio_device_ui/devicesmenu.h"
#include "audio_device_ui/outputdevicename.h"
#include "icons.h"

#include <QEvent>
#include <QFontMetrics>
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

  // Sizes the button from an ASCII sample, so device names carrying fallback-font
  // glyphs (e.g. "MKⅢ") cannot inflate it out of line with the rest of the row.
  void OutputDeviceButton::updateSize() {
    const QString sample(name_max_chars, u'A');
    button->setText(sample);
    button->setFixedSize(button->sizeHint());
    text_width = button->fontMetrics().horizontalAdvance(sample);
    refresh();
  }

  void OutputDeviceButton::refresh() {
    const QString name = outputDeviceName(local_conf.outputDeviceId());
    button->setText(button->fontMetrics().elidedText(name, Qt::ElideRight, text_width));
    button->setToolTip(name);
  }

  void OutputDeviceButton::on_buttonClicked() {
    DevicesMenu menu(button, local_conf);
    connect(&menu, &DevicesMenu::outputDeviceChanged, this, &OutputDeviceButton::outputDeviceChanged);
    int x = button->width() - menu.sizeHint().width();
    QPoint pos(button->mapToGlobal(QPoint(x, button->height())));
    menu.exec(pos);
    refresh();
  }
} // namespace AudioDeviceUi
