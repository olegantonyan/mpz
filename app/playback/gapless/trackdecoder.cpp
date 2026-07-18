#include "playback/gapless/trackdecoder.h"

#include <QAudioBuffer>

namespace Playback::Gapless {
  TrackDecoder::TrackDecoder(QObject *parent) : QObject(parent) {
    connect(&decoder, &QAudioDecoder::bufferReady, this, &TrackDecoder::pump);
    connect(&decoder, &QAudioDecoder::formatChanged, this,
            [this](const QAudioFormat &format) { emitFormatOnce(format); });
    connect(&decoder, &QAudioDecoder::finished, this,
            [this]() { emit finished(current_url, total_frames); });
    connect(&decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this,
            [this](QAudioDecoder::Error) { emit decodeError(current_url, decoder.errorString()); });
  }

  void TrackDecoder::start(const QUrl &url) {
    current_url = url;
    format_emitted = false;
    total_frames = 0;
    paused = false;
    decoder.setSource(url);
    if (requested_format.isValid()) {
      decoder.setAudioFormat(requested_format);
    }
    decoder.start();
  }

  void TrackDecoder::stop() {
    decoder.stop();
  }

  void TrackDecoder::requestFormat(const QAudioFormat &format) {
    requested_format = format;
  }

  void TrackDecoder::setPaused(bool p) {
    const bool was_paused = paused;
    paused = p;
    if (was_paused && !paused) {
      pump();
    }
  }

  void TrackDecoder::emitFormatOnce(const QAudioFormat &format) {
    if (format_emitted) {
      return;
    }
    format_emitted = true;
    emit firstFormat(format);
  }

  void TrackDecoder::pump() {
    while (!paused && decoder.bufferAvailable()) {
      const QAudioBuffer buffer = decoder.read();
      if (!buffer.isValid()) {
        break;
      }
      const QAudioFormat format = buffer.format();
      emitFormatOnce(format);
      const int bytes_per_frame = format.bytesPerFrame();
      if (bytes_per_frame > 0) {
        total_frames += buffer.byteCount() / bytes_per_frame;
      }
      emit pcm(current_url, QByteArray(buffer.constData<char>(), buffer.byteCount()));
    }
  }
}
