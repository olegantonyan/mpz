#ifndef CONTROLS_H
#define CONTROLS_H

#include <QToolButton>
#include <QProgressBar>
#include <QLabel>

namespace Playback {
  class Controls {
  public:
    explicit Controls();

    QToolButton *stop;
    QToolButton *play;
    QToolButton *pause;
    QToolButton *prev;
    QToolButton *next;
    QProgressBar *seekbar;
    QLabel *time;
  };
}

#endif // CONTROLS_H
