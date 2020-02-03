#include "decoder.h"

#include <QDebug>
#include <QDateTime>

namespace Audio {
  Decoder::Decoder() {
    qDebug()  << QDateTime::currentDateTime().toString() << "load file begin";
    audioFile.load("/home/oleg/Desktop/file.wav");
    qDebug()  << QDateTime::currentDateTime().toString() << "load end begin";
    audioFile.printSummary();
    sample_pointer = 0;

    _format.setSampleRate(44100);
    _format.setChannelCount(2);
    _format.setCodec("audio/pcm");
    //_format.setSampleType()
    //_format.setByteOrder()
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

  const char *Decoder::streamName() const {
    // TODO
    return "audio playback";
  }

  const QAudioFormat &Decoder::format() const {
    return _format;
  }
}
