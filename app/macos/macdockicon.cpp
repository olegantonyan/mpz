#include "macdockicon.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>

namespace {
  const int FRAME_COUNT = 24;
  const int FRAME_INTERVAL_MS = 75; // 24 frames of 15 degrees every 75ms == 33 1/3 rpm
  const int ICON_SIZE = 256;        // dock tile and cmd-tab switcher top out at 128pt
}

MacDockIcon::MacDockIcon(Config::Global &global_c, QObject *parent) : QObject(parent), global_conf(global_c) {
  timer.setInterval(FRAME_INTERVAL_MS);
  connect(&timer, &QTimer::timeout, this, &MacDockIcon::tick);
}

void MacDockIcon::start() {
  if (!enabled()) {
    return;
  }
  buildFrames();
  if (frames.isEmpty()) {
    return;
  }
  timer.start();
}

void MacDockIcon::pause() {
  timer.stop();
}

void MacDockIcon::stop() {
  timer.stop();
  frame = 0;
  QGuiApplication::setWindowIcon(QIcon()); // back to the bundle icon
}

void MacDockIcon::tick() {
  if (!enabled()) { // picked up here so switching the setting off stops the current spin
    stop();
    return;
  }
  frame = (frame + 1) % frames.size();
  QGuiApplication::setWindowIcon(frames.at(frame));
}

bool MacDockIcon::enabled() const {
  return !global_conf.disableDockIconAnimation();
}

void MacDockIcon::buildFrames() {
  if (!frames.isEmpty()) {
    return;
  }

  QPixmap base(":/app/resources/icons/dock/mpz-base.png");
  QPixmap dots(":/app/resources/icons/dock/mpz-dots.png");
  if (base.isNull() || dots.isNull()) {
    qWarning() << "dock icon layers missing, not animating";
    return;
  }
  base = base.scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  dots = dots.scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  frames.reserve(FRAME_COUNT);
  for (int i = 0; i < FRAME_COUNT; i++) {
    QPixmap f = base;
    QPainter p(&f);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.translate(ICON_SIZE / 2.0, ICON_SIZE / 2.0);
    p.rotate(i * 360.0 / FRAME_COUNT);
    p.translate(-ICON_SIZE / 2.0, -ICON_SIZE / 2.0);
    p.drawPixmap(0, 0, dots);
    p.end();
    frames.append(QIcon(f));
  }
}
