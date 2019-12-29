#include "trayicon.h"

#include <QAction>
#include <QDebug>

TrayIcon::TrayIcon(QMainWindow *parent) : QObject(parent) {
  trayicon = new QSystemTrayIcon(parent->windowIcon(), parent);
  trayicon->setToolTip("Stopped");
  menu = new QMenu(parent);

  quit = new QAction("Quit", this);
  play = new QAction("Play", this);
  pause = new QAction("Pause", this);
  stop = new QAction("Stop", this);
  next = new QAction("Next", this);
  prev = new QAction("Previous", this);
  now_playing = new QAction("", this);
  now_playing->setEnabled(false);
  connect(play, &QAction::triggered, this, &TrayIcon::startTriggered);
  connect(pause, &QAction::triggered, this, &TrayIcon::pauseTriggered);
  connect(stop, &QAction::triggered, this, &TrayIcon::stopTriggered);
  connect(next, &QAction::triggered, this, &TrayIcon::nextTriggered);
  connect(prev, &QAction::triggered, this, &TrayIcon::prevTriggered);
  connect(quit, &QAction::triggered, parent, &QMainWindow::close);

  menu->addAction(now_playing);
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
}

void TrayIcon::hide() {
  trayicon->hide();
}

void TrayIcon::on_playerStarted(const Track &track) {
  Q_UNUSED(track)
  play->setEnabled(false);
  stop->setEnabled(true);
  pause->setEnabled(true);
  next->setEnabled(true);
  prev->setEnabled(true);
  update_menu_now_playing(track, 0);
  trayicon->setToolTip(QString("Playing: %1 - %2").arg(track.artist()).arg(track.title()));
}

void TrayIcon::on_playerStopped() {
  play->setEnabled(true);
  stop->setEnabled(false);
  pause->setEnabled(false);
  next->setEnabled(false);
  prev->setEnabled(false);
  update_menu_now_playing(Track(), -1);
  trayicon->setToolTip("Stopped");
}

void TrayIcon::on_playerPaused(const Track &track) {
  Q_UNUSED(track)
  play->setEnabled(true);
  stop->setEnabled(true);
  pause->setEnabled(true);
  next->setEnabled(true);
  prev->setEnabled(true);
  trayicon->setToolTip(QString("Paused: %1 - %2").arg(track.artist()).arg(track.title()));
}

void TrayIcon::on_playerProgress(const Track &track, int current_seconds) {
  update_menu_now_playing(track, current_seconds);
}

QString TrayIcon::time_text(const Track &track, int pos) const {
  return QString("%1/%2").arg(Track::formattedTime(static_cast<quint32>(pos))).arg(track.formattedDuration());
}

void TrayIcon::update_menu_now_playing(const Track &track, int pos) {
  if (pos < 0) {
    now_playing->setText("");
    return;
  }
  auto time_t = time_text(track, pos);
  auto track_t = track.title();
  auto t = QString("%1 (%2)").arg(track_t).arg(time_t);
  now_playing->setText(t);
}
