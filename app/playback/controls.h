#ifndef CONTROLS_H
#define CONTROLS_H

#include <QToolButton>
#include <QProgressBar>
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
    QProgressBar *seekbar = nullptr;
    QLabel *time = nullptr;
  };
}

#endif // CONTROLS_H
