#ifndef MACDOCKMENU_H
#define MACDOCKMENU_H

#include "playback/playbackcontroller.h"

#include <QObject>

class QMenu;
class QAction;

class MacDockMenu : public QObject {
  Q_OBJECT
public:
  explicit MacDockMenu(Playback::Controller *player, QWidget *parent);

private slots:
  void on_started(const Track &track);
  void on_paused(const Track &track);
  void on_stopped();

private:
  void updateNowPlaying(const Track &track);

  Playback::Controller *player;
  QMenu *menu;
  QAction *now_playing;
  QAction *play;
  QAction *pause;
  QAction *stop;
  QAction *next;
  QAction *prev;
};

#endif // MACDOCKMENU_H
