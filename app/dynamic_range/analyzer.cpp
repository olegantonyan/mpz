#include "dynamic_range/analyzer.h"

#include "decode/nativeformat.h"

#include <QAudioBuffer>
#include <QFileInfo>
#include <QUrl>

#include <limits>

namespace DynamicRange {
  namespace {
    constexpr int PROGRESS_INTERVAL_MS = 200;

    double normalize(float v) { return v; }
    double normalize(qint16 v) { return v / 32768.0; }
    double normalize(qint32 v) { return v / 2147483648.0; }
    double normalize(quint8 v) { return (int(v) - 128) / 128.0; }

    template <typename T>
    bool toDouble(const QAudioBuffer &buffer, QVector<double> &out) {
      const T *src = buffer.constData<T>();
      if (src == nullptr) {
        return false;
      }
      const qint64 count = qint64(buffer.frameCount()) * buffer.format().channelCount();
      out.resize(count);
      double *dst = out.data();
      for (qint64 i = 0; i < count; ++i) {
        dst[i] = normalize(src[i]);
      }
      return true;
    }

    qint64 usToFrames(qint64 us, int rate) {
      return us * qint64(rate) / 1000000;
    }
  }

  Analyzer::Analyzer(QObject *parent) : QObject(parent) {
  }

  // public mutators self-dispatch to the decoder's thread, so the blocking probe
  // inside QAudioDecoder::start() cannot freeze the caller
  void Analyzer::run(const QVector<Job> &jobs) {
    QMetaObject::invokeMethod(this, [this, jobs]() {
      cancelled.store(false);
      queue = jobs;
      current = -1;
      finished_us = 0;
      total_us = 0;
      for (const auto &j : queue) {
        for (const auto &s : j.segments) {
          total_us += s.duration_us;
        }
      }
      running = true;
      progress_timer.start();
      next();
    });
  }

  void Analyzer::cancel() {
    cancelled.store(true);
    QMetaObject::invokeMethod(this, [this]() {
      if (!running) {
        return;
      }
      releaseDecoder();
      job_active = false;
      running = false;
      emit finished(true);
    });
  }

  void Analyzer::next() {
    if (!running) {
      return;
    }
    if (cancelled.load()) {
      running = false;
      emit finished(true);
      return;
    }
    ++current;
    if (current >= queue.size()) {
      running = false;
      emit finished(false);
      return;
    }
    startJob();
  }

  void Analyzer::startJob() {
    const Job &job = queue.at(current);
    segments = job.segments;
    accs.clear();
    scratch.clear();
    format_known = false;
    channels = 0;
    sample_rate = 0;
    frame_pos = 0;
    job_active = true;
    abort_queued = false;

    // Qt 6.4's gstreamer decoder reports neither error nor finished for a file it
    // cannot open, which leaves the scan stuck on that job
    if (!QFileInfo::exists(job.path)) {
      finishJob();
      QMetaObject::invokeMethod(this, [this]() { next(); }, Qt::QueuedConnection);
      return;
    }

    // a decoder per job: reusing one lets a stale finished() from the previous file
    // terminate the next job before it produces a single buffer
    decoder = new QAudioDecoder(this);
    connect(decoder, &QAudioDecoder::bufferReady, this, &Analyzer::onBufferReady);
    connect(decoder, &QAudioDecoder::finished, this, &Analyzer::onDecoderFinished);
    connect(decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this,
            [this](QAudioDecoder::Error) { onDecodeError(); });
    decoder->setSource(QUrl::fromLocalFile(job.path));
    decoder->setAudioFormat(Decode::nativeAudioFormat(job.path));
    decoder->start();
  }

  void Analyzer::releaseDecoder() {
    if (decoder == nullptr) {
      return;
    }
    decoder->disconnect(this);
    decoder->stop();
    decoder->deleteLater();
    decoder = nullptr;
  }

  void Analyzer::abortJob() {
    if (!job_active) {
      return;
    }
    finishJob();
    next();
  }

  void Analyzer::finishJob() {
    if (!job_active) {
      return;
    }
    job_active = false;
    releaseDecoder();
    for (int i = 0; i < segments.size(); ++i) {
      Result r;
      if (i < accs.size()) {
        r = accs[i].finish();
      }
      emit segmentDone(segments.at(i).uid, r);
      finished_us += segments.at(i).duration_us;
    }
    emit progress(finished_us, total_us);
  }

  void Analyzer::onBufferReady() {
    while (decoder != nullptr && !cancelled.load() && decoder->bufferAvailable()) {
      const QAudioBuffer buffer = decoder->read();
      if (!buffer.isValid()) {
        break;
      }
      consume(buffer);
    }
    emitProgress();
    if (job_active && !abort_queued && format_known && everySegmentCovered()) {
      abort_queued = true;
      QMetaObject::invokeMethod(this, [this]() { abortJob(); }, Qt::QueuedConnection);
    }
  }

  void Analyzer::onDecoderFinished() {
    if (!running || !job_active) {
      return;
    }
    onBufferReady();
    if (!job_active) {
      return;
    }
    finishJob();
    QMetaObject::invokeMethod(this, [this]() { next(); }, Qt::QueuedConnection);
  }

  void Analyzer::onDecodeError() {
    if (!running || !job_active) {
      return;
    }
    finishJob();
    QMetaObject::invokeMethod(this, [this]() { next(); }, Qt::QueuedConnection);
  }

  void Analyzer::consume(const QAudioBuffer &buffer) {
    const QAudioFormat format = buffer.format();
    const int ch = format.channelCount();
    const int rate = format.sampleRate();
    const qint64 frames = buffer.frameCount();
    if (ch <= 0 || rate <= 0 || frames <= 0) {
      return;
    }

    if (!format_known) {
      format_known = true;
      channels = ch;
      sample_rate = rate;
      accs.clear();
      accs.reserve(segments.size());
      for (int i = 0; i < segments.size(); ++i) {
        accs.append(Accumulator(ch, rate));
      }
    } else if (ch != channels) {
      for (auto &a : accs) {
        a.invalidate();
      }
      return;
    } else if (rate != sample_rate) {
      sample_rate = rate;
      for (auto &a : accs) {
        a.setSampleRate(rate);
      }
    }

    bool converted = false;
    switch (format.sampleFormat()) {
      case QAudioFormat::Float:
        converted = toDouble<float>(buffer, scratch);
        break;
      case QAudioFormat::Int16:
        converted = toDouble<qint16>(buffer, scratch);
        break;
      case QAudioFormat::Int32:
        converted = toDouble<qint32>(buffer, scratch);
        break;
      case QAudioFormat::UInt8:
        converted = toDouble<quint8>(buffer, scratch);
        break;
      default:
        break;
    }
    if (!converted) {
      return;
    }

    const qint64 buffer_begin = frame_pos;
    const qint64 buffer_end = frame_pos + frames;
    for (int i = 0; i < segments.size() && i < accs.size(); ++i) {
      const Segment &s = segments.at(i);
      const qint64 begin = usToFrames(s.begin_us, sample_rate);
      const qint64 end = s.end_us < 0 ? std::numeric_limits<qint64>::max()
                                      : usToFrames(s.end_us, sample_rate);
      const qint64 from = qMax(buffer_begin, begin);
      const qint64 to = qMin(buffer_end, end);
      if (to <= from) {
        continue;
      }
      accs[i].addInterleaved(scratch.constData() + (from - buffer_begin) * channels, to - from);
    }
    frame_pos = buffer_end;
  }

  bool Analyzer::everySegmentCovered() const {
    for (const auto &s : segments) {
      if (s.end_us < 0 || frame_pos < usToFrames(s.end_us, sample_rate)) {
        return false;
      }
    }
    return !segments.isEmpty();
  }

  void Analyzer::emitProgress() {
    if (progress_timer.isValid() && progress_timer.elapsed() < PROGRESS_INTERVAL_MS) {
      return;
    }
    progress_timer.restart();
    qint64 done = finished_us;
    if (sample_rate > 0) {
      const qint64 pos_us = frame_pos * 1000000 / sample_rate;
      for (const auto &s : segments) {
        done += qBound(qint64(0), pos_us - s.begin_us, s.duration_us);
      }
    }
    emit progress(done, total_us);
  }
}
