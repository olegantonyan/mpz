#ifndef CONTROLS_H
#define CONTROLS_H

#include <QToolButton>
#include <QProgressBar>

namespace Playback {
  class Controls {
  public:
    Controls();

    QToolButton *stop;
    QToolButton *play;
    QToolButton *pause;
    QToolButton *prev;
    QToolButton *next;
    QProgressBar *seekbar;
  };
}

#endif // CONTROLS_H
