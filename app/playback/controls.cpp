#include "controls.h"

namespace Playback {
  Controls::Controls() {
    stop = nullptr;
    play = nullptr;
    pause = nullptr;
    prev = nullptr;
    next = nullptr;
    seekbar = nullptr;
  }
}
