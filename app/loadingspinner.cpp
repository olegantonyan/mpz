#include "loadingspinner.h"
#include "icons.h"

#include <QPainter>
#include <QTimer>

LoadingSpinner::LoadingSpinner(QWidget *parent, bool center_on_parent)
    : QWidget(parent), timer(new QTimer(this)), angle(0), center_on_parent(center_on_parent) {
  setFixedSize(18, 18);
  hide();

  timer->setInterval(33);
  connect(timer, &QTimer::timeout, this, [this]() {
    angle = (angle + 12) % 360;
    update();
  });
}

void LoadingSpinner::start() {
  updatePosition();
  show();
  if (!timer->isActive()) {
    timer->start();
  }
}

void LoadingSpinner::stop() {
  timer->stop();
  hide();
}

void LoadingSpinner::updatePosition() {
  if (center_on_parent && parentWidget()) {
    move(parentWidget()->width() / 2 - width() / 2, parentWidget()->height() / 2 - height() / 2);
  }
}

void LoadingSpinner::paintEvent(QPaintEvent *) {
  updatePosition();

  const QPixmap pm = Icons::pixmap(Icons::Icon::Spinner, size());
  if (pm.isNull()) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.translate(width() / 2.0, height() / 2.0);
  painter.rotate(angle);
  painter.translate(-width() / 2.0, -height() / 2.0);
  painter.drawPixmap(0, 0, pm);
}
