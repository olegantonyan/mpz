#ifndef ANALYZER_H
#define ANALYZER_H

#include "diskcache.h"
#include "waveform/peaks.h"

#include <QAudioDecoder>
#include <QElapsedTimer>
#include <QObject>
#include <QString>

namespace Waveform {
  class Analyzer : public QObject {
    Q_OBJECT

  public:
    explicit Analyzer(QObject *parent = nullptr);

    void request(const QString &filepath);
    void cancel();

  signals:
    void ready(const QString &filepath, const Waveform::Peaks &peaks);

  private:
    void startDecode();
    void onBufferReady();
    void onFinished();
    void accumulate(double sample);
    void flushBucket();
    void maybeEmitPartial();
    void resetAccumulator();

    QAudioDecoder decoder;
    DiskCache::Store store;
    QString current_path;
    QString current_key;
    Peaks accumulated;
    QElapsedTimer partial_timer;
    qint64 emitted_buckets = 0;
    qint64 bucket_frames = 0;
    qint64 bucket_count = 0;
    double bucket_peak = 0.0;
    double bucket_sq = 0.0;
  };
}

#endif // ANALYZER_H
