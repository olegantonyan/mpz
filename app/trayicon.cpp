#include "trayicon.h"

#include <QAction>
#include <QDebug>
#include <QStyle>

TrayIcon::TrayIcon(QMainWindow *parent, Config::Global &global_c) : QObject(parent), global_conf(global_c) {
  trayicon = new QSystemTrayIcon(parent->windowIcon(), parent);
  trayicon->setToolTip(tr("Stopped"));
  menu = new QMenu(parent);

  quit = new QAction(tr("Quit"), this);

  play = new QAction(tr("Play"), this);
  play->setIcon(parent->style()->standardIcon(QStyle::SP_MediaPlay));
  pause = new QAction(tr("Pause"), this);
  pause->setIcon(parent->style()->standardIcon(QStyle::SP_MediaPause));
  stop = new QAction(tr("Stop"), this);
  stop->setIcon(parent->style()->standardIcon(QStyle::SP_MediaStop));
  next = new QAction(tr("Next"), this);
  next->setIcon(parent->style()->standardIcon(QStyle::SP_MediaSeekForward));
  prev = new QAction(tr("Previous"), this);
  prev->setIcon(parent->style()->standardIcon(QStyle::SP_MediaSeekBackward));
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
    if (global_conf.minimizeToTray()) {
      emit clicked();
    } else if (reason == QSystemTrayIcon::Trigger) {
      menu->popup(QCursor::pos());
    }
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
  trayicon->setToolTip(QString("%1: %2 - %3").arg(tr("Playing")).arg(track.artist()).arg(track.title()));
}

void TrayIcon::on_playerStopped() {
  play->setEnabled(true);
  stop->setEnabled(false);
  pause->setEnabled(false);
  next->setEnabled(false);
  prev->setEnabled(false);
  update_menu_now_playing(Track(), -1);
  trayicon->setToolTip(tr("Stopped"));
}

void TrayIcon::on_playerPaused(const Track &track) {
  Q_UNUSED(track)
  play->setEnabled(true);
  stop->setEnabled(true);
  pause->setEnabled(true);
  next->setEnabled(true);
  prev->setEnabled(true);
  trayicon->setToolTip(QString("%1: %2 - %3").arg(tr("Paused")).arg(track.artist()).arg(track.title()));
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
  QString track_t = track.shortText();
  auto t = QString("%1 (%2)").arg(track_t).arg(time_t);
  now_playing->setText(t);
}
