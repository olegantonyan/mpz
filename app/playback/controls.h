#ifndef CONTROLS_H
#define CONTROLS_H

#include <QToolButton>
#include <QSlider>

namespace Playback {
  class Controls {
  public:
    Controls();

    QToolButton *stop;
    QToolButton *play;
    QToolButton *pause;
    QToolButton *prev;
    QToolButton *next;
    QSlider *seekbar;
  };
}

#endif // CONTROLS_H
