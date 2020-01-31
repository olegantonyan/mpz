#ifndef DECODER_H
#define DECODER_H

extern "C" {
#include "libavresample/avresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
}

namespace Audio {
  class Decoder {
  public:
    Decoder();
  };
}


#endif // DECODER_H
