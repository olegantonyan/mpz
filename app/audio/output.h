#ifndef OUTPUT_H
#define OUTPUT_H

#include "audio/decoder.h"

extern "C" {
#include "soundio/soundio.h"
}

namespace Audio {
  namespace OutputPrivate {
    class Context {
    public:
      explicit Context(Decoder *decoder);

      bool pause() const;
      void setPause(bool v);

      void (*write_sample)(char *ptr, double sample);

      double readSample(int channel);

    private:
      Decoder *decoder;
      bool _pause;
    };
  }

  class Output {
  public:
    explicit Output(Decoder *decoder);
    ~Output();

  private:
    bool init();
    bool connect_backend(enum SoundIoBackend backend);

    struct SoundIo *soundio;
    struct SoundIoDevice *device;
    struct SoundIoOutStream *outstream;

    OutputPrivate::Context context;
  };
}

#endif // OUTPUT_H
