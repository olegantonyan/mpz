#ifndef OUTPUT_H
#define OUTPUT_H

#include "audio/decoder.h"

namespace Audio {
  class Output {
  public:
    explicit Output(Decoder *decoder);
    ~Output();

  private:
    Decoder *decoder;
  };
}

#endif // OUTPUT_H
