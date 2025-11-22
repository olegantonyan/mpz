#include "slidingbanner.h"

#include <QVBoxLayout>
#include <QEasingCurve>

SlidingBanner::SlidingBanner(QWidget *parent) : QWidget{parent} {
  setFixedHeight(0);

  label = new QLabel(this);
  label->setAlignment(Qt::AlignCenter);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(label);

  animation = new QPropertyAnimation(this, "maximumHeight");
  animation->setDuration(250);
  animation->setEasingCurve(QEasingCurve::OutCubic);

  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &SlidingBanner::collapse);
}

void SlidingBanner::showMessage(const QString &text, BannerType type, int timeoutMs) {
  label->setText(text);

  switch(type) {
   case Success:
     setStyleSheet("background:#4CAF50; color:white; padding:6px 12px; border-radius:4px;");
     break;
   case Error:
     setStyleSheet("background:#FF4C4C; color:white; padding:6px 12px; border-radius:4px;");
     break;
  }

  expand();

  if (timeoutMs > 0) {
    timer.setInterval(timeoutMs);
    timer.start();
  } else {
    timer.stop();
  }
}

void SlidingBanner::expand() {
  animation->stop();
  animation->setStartValue(height());
  animation->setEndValue(bannerHeight);
  animation->start();
}

void SlidingBanner::collapse() {
  animation->stop();
  animation->setStartValue(height());
  animation->setEndValue(0);
  animation->start();
}
