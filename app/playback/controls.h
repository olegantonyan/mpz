#ifndef CONTROLS_H
#define CONTROLS_H

#include "playback/seekbar.h"

#include <QToolButton>
#include <QLabel>

namespace Playback {
  class Controls {
  public:
    explicit Controls();

    QToolButton *stop = nullptr;
    QToolButton *play = nullptr;
    QToolButton *pause = nullptr;
    QToolButton *prev = nullptr;
    QToolButton *next = nullptr;
    Seekbar *seekbar = nullptr;
    QLabel *time = nullptr;
  };
}

#endif // CONTROLS_H
