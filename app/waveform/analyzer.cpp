#include "waveform/analyzer.h"

#include "decode/nativeformat.h"

#include <QAudioBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

#include <cmath>

namespace Waveform {
  namespace {
    constexpr quint16 BUCKET_MS = 20;
    constexpr int PARTIAL_INTERVAL_MS = 100;
    const QString EXT = QStringLiteral("wf");
    constexpr int MAX_CACHE_ENTRIES = 500;

    double normalize(float v) { return v; }
    double normalize(qint16 v) { return v / 32768.0; }
    double normalize(qint32 v) { return v / 2147483648.0; }
    double normalize(quint8 v) { return (int(v) - 128) / 128.0; }

    template <typename T, typename Sink>
    void reduceFrames(const QAudioBuffer &buffer, Sink sink) {
      const T *src = buffer.constData<T>();
      if (src == nullptr) {
        return;
      }
      const int channels = buffer.format().channelCount();
      const int frames = buffer.frameCount();
      for (int f = 0; f < frames; ++f) {
        double v = 0.0;
        for (int c = 0; c < channels; ++c) {
          v += normalize(src[f * channels + c]);
        }
        sink(v / channels);
      }
    }

    quint8 toByte(double v) {
      return static_cast<quint8>(qBound(0.0, v, 1.0) * 255.0 + 0.5);
    }

    QString cacheKey(const QFileInfo &info) {
      const QByteArray raw = (info.absoluteFilePath() + QChar(0x1F) +
                              QString::number(info.lastModified().toSecsSinceEpoch()) + QChar(0x1F) +
                              QString::number(info.size()))
                                 .toUtf8();
      return QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex());
    }
  }

  Analyzer::Analyzer(QObject *parent) : QObject(parent), store("waveform") {
    connect(&decoder, &QAudioDecoder::bufferReady, this, &Analyzer::onBufferReady);
    connect(&decoder, &QAudioDecoder::finished, this, &Analyzer::onFinished);
    connect(&decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this,
            [this](QAudioDecoder::Error) { cancel(); });
    store.trim({EXT}, MAX_CACHE_ENTRIES);
  }

  void Analyzer::request(const QString &filepath) {
    const QFileInfo info(filepath);
    if (filepath.isEmpty() || !info.isFile()) {
      cancel();
      return;
    }

    const QString key = cacheKey(info);
    if (key == current_key) {
      return;
    }
    cancel();
    current_path = filepath;
    current_key = key;

    QFile cached(store.find(key, {EXT}));
    if (cached.exists() && cached.open(QIODevice::ReadOnly)) {
      const Peaks peaks = Peaks::deserialize(cached.readAll());
      if (!peaks.isEmpty()) {
        emit ready(filepath, peaks);
        return;
      }
    }
    startDecode();
  }

  void Analyzer::cancel() {
    decoder.stop();
    current_path.clear();
    current_key.clear();
    resetAccumulator();
  }

  void Analyzer::startDecode() {
    // FLAC only: a requested format wedges the decoder on mp3s with an encoder delay
    partial_timer.start();
    decoder.setSource(QUrl::fromLocalFile(current_path));
    decoder.setAudioFormat(Decode::nativeAudioFormat(current_path));
    decoder.start();
  }

  void Analyzer::onBufferReady() {
    while (decoder.bufferAvailable()) {
      const QAudioBuffer buffer = decoder.read();
      if (!buffer.isValid()) {
        break;
      }
      const QAudioFormat format = buffer.format();
      if (format.channelCount() <= 0 || buffer.frameCount() <= 0) {
        continue;
      }

      if (bucket_frames == 0) {
        if (format.sampleRate() <= 0) {
          continue;
        }
        bucket_frames = qMax(qint64(1), qint64(format.sampleRate()) * BUCKET_MS / 1000);
        accumulated.bucket_ms = BUCKET_MS;
      }

      auto sink = [this](double v) { accumulate(v); };
      switch (format.sampleFormat()) {
        case QAudioFormat::Float:
          reduceFrames<float>(buffer, sink);
          break;
        case QAudioFormat::Int16:
          reduceFrames<qint16>(buffer, sink);
          break;
        case QAudioFormat::Int32:
          reduceFrames<qint32>(buffer, sink);
          break;
        case QAudioFormat::UInt8:
          reduceFrames<quint8>(buffer, sink);
          break;
        default:
          break;
      }
    }
    maybeEmitPartial();
  }

  void Analyzer::maybeEmitPartial() {
    if (accumulated.peak.size() <= emitted_buckets) {
      return;
    }
    if (emitted_buckets > 0 && partial_timer.elapsed() < PARTIAL_INTERVAL_MS) {
      return;
    }
    emitted_buckets = accumulated.peak.size();
    partial_timer.restart();
    const qint64 scanned = emitted_buckets * accumulated.bucket_ms;
    accumulated.duration_ms = quint64(qMax(decoder.duration(), scanned));
    emit ready(current_path, accumulated);
  }

  void Analyzer::accumulate(double sample) {
    const double a = std::fabs(sample);
    if (a > bucket_peak) {
      bucket_peak = a;
    }
    bucket_sq += sample * sample;
    ++bucket_count;
    if (bucket_count >= bucket_frames) {
      flushBucket();
    }
  }

  void Analyzer::flushBucket() {
    if (bucket_count <= 0) {
      return;
    }
    accumulated.peak.append(toByte(bucket_peak));
    accumulated.rms.append(toByte(std::sqrt(bucket_sq / bucket_count)));
    bucket_peak = 0.0;
    bucket_sq = 0.0;
    bucket_count = 0;
  }

  void Analyzer::onFinished() {
    flushBucket();
    accumulated.duration_ms = quint64(accumulated.peak.size()) * accumulated.bucket_ms;
    if (accumulated.isEmpty()) {
      resetAccumulator();
      return;
    }

    const Peaks peaks = accumulated;
    const QString path = current_path;
    store.write(current_key, EXT, peaks.serialize());
    resetAccumulator();
    emit ready(path, peaks);
  }

  void Analyzer::resetAccumulator() {
    accumulated = Peaks();
    emitted_buckets = 0;
    bucket_frames = 0;
    bucket_count = 0;
    bucket_peak = 0.0;
    bucket_sq = 0.0;
  }
}
