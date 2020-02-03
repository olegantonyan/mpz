#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include "audio/output.h"
#include "audio/decoder.h"

namespace Audio {
  class Player {
  public:
    explicit Player();

    void play();
  };
}

#endif // AUDIO_PLAYER_H
