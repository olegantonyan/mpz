#ifndef DECODER_H
#define DECODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
}

#include <QAudioFormat>

namespace Audio {
  class Decoder {
  public:
    explicit Decoder();

    double readSample(bool *eof, int channel, int channels_count);

    const char *streamName() const;

    const QAudioFormat& format() const;

  private:
    QAudioFormat _format;

    unsigned long sample_pointer;
  };
}


#endif // DECODER_H
