#ifndef OUTPUT_H
#define OUTPUT_H

#include "audio/decoder.h"

extern "C" {
#include "soundio/soundio.h"
}

namespace Audio {
  class Output {
  public:
    explicit Output(Decoder *decoder);
    ~Output();

    bool paused() const;
    void pause(bool v);

    void (*write_sample)(char *ptr, double sample);

    double readSample(int channel, int channels_count);

  private:
    bool init();
    void deinit();
    bool connect_backend(enum SoundIoBackend backend);

    struct SoundIo *soundio;
    struct SoundIoDevice *device;
    struct SoundIoOutStream *outstream;

    Decoder *decoder;
    bool _pause;
    bool _stop;
  };
}

#endif // OUTPUT_H
