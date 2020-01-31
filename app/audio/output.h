#ifndef OUTPUT_H
#define OUTPUT_H

extern "C" {
#include "soundio/soundio.h"
}

namespace Audio {
  class Output {
  public:
    Output();

    void test();
  };
}

#endif // OUTPUT_H
