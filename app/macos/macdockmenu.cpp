#include "macdockmenu.h"

#include "track.h"

#include <QMenu>
#include <QAction>
#include <QToolButton>

MacDockMenu::MacDockMenu(Playback::Controller *pl, QWidget *parent) : QObject(parent), player(pl) {
  menu = new QMenu(parent);

  now_playing = menu->addAction("");
  now_playing->setEnabled(false);
  menu->addSeparator();
  play = menu->addAction(tr("Play"));
  pause = menu->addAction(tr("Pause"));
  stop = menu->addAction(tr("Stop"));
  next = menu->addAction(tr("Next"));
  prev = menu->addAction(tr("Previous"));

  connect(play, &QAction::triggered, player->controls().play, &QToolButton::click);
  connect(pause, &QAction::triggered, player->controls().pause, &QToolButton::click);
  connect(stop, &QAction::triggered, player->controls().stop, &QToolButton::click);
  connect(next, &QAction::triggered, player->controls().next, &QToolButton::click);
  connect(prev, &QAction::triggered, player->controls().prev, &QToolButton::click);

  connect(player, &Playback::Controller::started, this, &MacDockMenu::on_started);
  connect(player, &Playback::Controller::paused, this, &MacDockMenu::on_paused);
  connect(player, &Playback::Controller::stopped, this, &MacDockMenu::on_stopped);
  connect(player, &Playback::Controller::trackChanged, this, [this](const Track &t) { updateNowPlaying(t); });

  on_stopped();

  menu->setAsDockMenu();
}

void MacDockMenu::on_started(const Track &track) {
  play->setEnabled(false);
  pause->setEnabled(true);
  stop->setEnabled(true);
  next->setEnabled(true);
  prev->setEnabled(true);
  updateNowPlaying(track);
}

void MacDockMenu::on_paused(const Track &track) {
  play->setEnabled(true);
  pause->setEnabled(true);
  stop->setEnabled(true);
  next->setEnabled(true);
  prev->setEnabled(true);
  updateNowPlaying(track);
}

void MacDockMenu::on_stopped() {
  play->setEnabled(true);
  pause->setEnabled(false);
  stop->setEnabled(false);
  next->setEnabled(false);
  prev->setEnabled(false);
  now_playing->setVisible(false);
}

void MacDockMenu::updateNowPlaying(const Track &track) {
  const QString text = track.shortText();
  now_playing->setVisible(!text.isEmpty());
  now_playing->setText(text);
}
