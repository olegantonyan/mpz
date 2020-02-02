#include "decoder.h"

#include <QDebug>

namespace Audio {
  Decoder::Decoder() {
    audioFile.load("/home/oleg/Desktop/file.wav");
    audioFile.printSummary();
    sample_pointer = 0;
  }

  double Decoder::readSample(bool *eof, int channel, int channels_count) {
    Q_ASSERT(eof);
    if (sample_pointer >= audioFile.getNumSamplesPerChannel()) {
      *eof = true;
      return 0.0;
    }

    double sample = audioFile.samples[channel][sample_pointer];
    if (channel + 1 >= channels_count) {
      sample_pointer++;
    }
    *eof = false;
    return sample;
  }
}
