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

  // public mutators self-dispatch to the decoder's thread (synchronous on the same
  // thread), so a stream decoder can live on a worker where the blocking probe
  // inside QAudioDecoder::start() cannot freeze the GUI
  void TrackDecoder::start(const QUrl &url) {
    QMetaObject::invokeMethod(this, [this, url]() {
      reset(url);
      decoder.setSource(url);
      startDecoder();
    });
  }

  void TrackDecoder::start(const QUrl &key, QIODevice *device) {
    QMetaObject::invokeMethod(this, [this, key, device]() {
      reset(key);
      decoder.setSourceDevice(device);
      startDecoder();
    });
  }

  void TrackDecoder::reset(const QUrl &key) {
    current_url = key;
    format_emitted = false;
    total_frames = 0;
    paused = false;
  }

  void TrackDecoder::startDecoder() {
    if (requested_format.isValid()) {
      decoder.setAudioFormat(requested_format);
    }
    decoder.start();
  }

  void TrackDecoder::stop() {
    QMetaObject::invokeMethod(this, [this]() { decoder.stop(); });
  }

  void TrackDecoder::requestFormat(const QAudioFormat &format) {
    QMetaObject::invokeMethod(this, [this, format]() { requested_format = format; });
  }

  void TrackDecoder::setPaused(bool p) {
    QMetaObject::invokeMethod(this, [this, p]() {
      const bool was_paused = paused;
      paused = p;
      if (was_paused && !paused) {
        pump();
      }
    });
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
