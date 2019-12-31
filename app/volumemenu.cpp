#include "volumemenu.h"

#include <QtGlobal>

VolumeMenu::VolumeMenu(QObject *parent) : QObject(parent), action(QWidgetAction(nullptr)) {
  slider.setMaximum(100);
  layout.addWidget(&slider);
  widget.setLayout(&layout);
  action.setDefaultWidget(&widget);
  menu.addAction(&action);
  connect(&slider, &QSlider::valueChanged, this, &VolumeMenu::changed);
}

void VolumeMenu::show(const QPoint &pos) {
  menu.exec(pos);
}

void VolumeMenu::setValue(int value) {
  slider.setValue(qMax(qMin(value, 100), 0));
}

QSize VolumeMenu::sizeHint() const {
  return menu.sizeHint();
}
