#ifndef OUTPUT_H
#define OUTPUT_H

#include "audio/decoder.h"

#include "audio/audiofile.h"

extern "C" {
#include "soundio/soundio.h"
}

namespace Audio {
  namespace OutputPrivate {
    class Context {
    public:
      explicit Context(Decoder *decoder);

      bool pause() const;
      bool stop() const;
      void setPause(bool v);
      void setStop();

      void (*write_sample)(char *ptr, double sample);

      double readSample(int channel, int channels_count);

    private:
      Decoder *decoder;
      bool _pause;
      bool _stop;

      AudioFile<double> audioFile;
      int sample_pointer;
    };
  }

  class Output {
  public:
    explicit Output(Decoder *decoder);
    ~Output();

  private:
    bool init();
    void deinit();
    bool connect_backend(enum SoundIoBackend backend);

    struct SoundIo *soundio;
    struct SoundIoDevice *device;
    struct SoundIoOutStream *outstream;

    OutputPrivate::Context context;
  };
}

#endif // OUTPUT_H
