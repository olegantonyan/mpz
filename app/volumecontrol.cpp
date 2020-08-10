#include "volumecontrol.h"

#include <QtGlobal>
#include <QWheelEvent>
#include <QStyle>

VolumeControl::VolumeControl(QToolButton *btn, int initial_value, QObject *parent) : QObject(parent), button(btn), menu(this) {
  connect(&menu, &PrivateVolumeControl::Menu::changed, this, &VolumeControl::changed);
  connect(button, &QToolButton::clicked, this, &VolumeControl::on_buttonClicked);
  button->installEventFilter(this);
  setValue(initial_value);
}

void VolumeControl::setValue(int value) {
  value = qBound(0, value, 100);
  menu.setValue(value);
  button->setText(QString("%1%").arg(value));
  if (value == 0) {
    button->setIcon(button->style()->standardIcon(QStyle::SP_MediaVolumeMuted));
  } else {
    button->setIcon(button->style()->standardIcon(QStyle::SP_MediaVolume));
  }
}

void VolumeControl::on_buttonClicked() {
  int menu_width = menu.sizeHint().width();
  int x = button->width() - menu_width;
  int y = button->height();
  QPoint pos(button->mapToGlobal(QPoint(x, y)));
  menu.show(pos);
}

bool VolumeControl::eventFilter(QObject *obj, QEvent *event) {
  if (obj == button) {
    if (event->type() == QEvent::Wheel) {
      QWheelEvent *we = dynamic_cast<QWheelEvent *>(event);
      if (we->angleDelta().y() > 0) {
        emit changed(menu.value() + 5);
      } else if (we->angleDelta().y() < 0) {
        emit changed(menu.value() - 5);
      }
    }
  }
  return QObject::eventFilter(obj, event);
}


namespace PrivateVolumeControl {
  Menu::Menu(QObject *parent) : QObject(parent), action(this) {
    slider.setMaximum(100);
    slider.setMinimum(0);
    layout.addWidget(&slider);
    widget.setLayout(&layout);
    action.setDefaultWidget(&widget);
    menu.addAction(&action);
    connect(&slider, &QSlider::valueChanged, this, &Menu::changed);
  }

  void Menu::show(const QPoint &pos) {
    menu.exec(pos);
  }

  void Menu::setValue(int value) {
    slider.setValue(value);
  }

  QSize Menu::sizeHint() const {
    return menu.sizeHint();
  }

  int Menu::value() const {
    return slider.value();
  }
}
