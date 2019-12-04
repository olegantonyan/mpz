#include "trayicon.h"

#include <QAction>

TrayIcon::TrayIcon(Playback::View *p, const QIcon &appicon, QMainWindow *parent) : QObject(parent), player(p) {
  trayicon = new QSystemTrayIcon(appicon, parent);
  menu = new QMenu(parent);

  QAction *quit = new QAction("Quit", this);
  QAction *play = new QAction("Play", this);
  QAction *pause = new QAction("Pause", this);
  QAction *stop = new QAction("Stop", this);
  QAction *next = new QAction("Next", this);
  QAction *prev = new QAction("Previous", this);
  QAction *now_plying = new QAction("", this);
  now_plying->setEnabled(false);
  connect(play, &QAction::triggered, player->controls().play, &QToolButton::click);
  connect(pause, &QAction::triggered, player->controls().pause, &QToolButton::click);
  connect(stop, &QAction::triggered, player->controls().stop, &QToolButton::click);
  connect(next, &QAction::triggered, player->controls().next, &QToolButton::click);
  connect(prev, &QAction::triggered, player->controls().prev, &QToolButton::click);
  connect(quit, &QAction::triggered, parent, &QMainWindow::close);
  menu->addAction(now_plying);
  menu->addSeparator();
  menu->addAction(play);
  menu->addAction(pause);
  menu->addAction(stop);
  menu->addAction(next);
  menu->addAction(prev);
  menu->addSeparator();
  menu->addAction(quit);
  trayicon->setContextMenu(menu);
  trayicon->show();
  connect(trayicon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason) {
    Q_UNUSED(reason)
    menu->popup(QCursor::pos());
  });

  connect(player, &Playback::View::started, [=](const Track &track) {
    Q_UNUSED(track)
    play->setEnabled(false);
    stop->setEnabled(true);
    pause->setEnabled(true);
    next->setEnabled(true);
    prev->setEnabled(true);
    now_plying->setText(track.title());
  });

  connect(player, &Playback::View::stopped, [=]() {
    play->setEnabled(true);
    stop->setEnabled(false);
    pause->setEnabled(false);
    next->setEnabled(false);
    prev->setEnabled(false);
  });

  connect(player, &Playback::View::paused, [=](const Track &track) {
    Q_UNUSED(track)
    play->setEnabled(true);
    stop->setEnabled(true);
    pause->setEnabled(true);
    next->setEnabled(true);
    prev->setEnabled(true);
  });
}

void TrayIcon::hide() {
  trayicon->hide();
}
