#ifndef DYNAMIC_RANGE_ANALYZER_H
#define DYNAMIC_RANGE_ANALYZER_H

#include "dynamic_range/dr.h"

#include <QAudioDecoder>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QVector>

#include <atomic>

namespace DynamicRange {
  struct Segment {
    quint64 uid = 0;
    qint64 begin_us = 0;
    qint64 end_us = -1;
    qint64 duration_us = 0;
  };

  struct Job {
    QString path;
    QVector<Segment> segments;
  };

  class Analyzer : public QObject {
    Q_OBJECT

  public:
    explicit Analyzer(QObject *parent = nullptr);

    void run(const QVector<DynamicRange::Job> &jobs);
    void cancel();

  signals:
    void progress(qint64 done_us, qint64 total_us);
    void segmentDone(quint64 uid, const DynamicRange::Result &result);
    void finished(bool cancelled);

  private:
    void next();
    void startJob();
    void abortJob();
    void finishJob();
    void releaseDecoder();
    void onBufferReady();
    void onDecoderFinished();
    void onDecodeError();
    void consume(const QAudioBuffer &buffer);
    void emitProgress();
    bool everySegmentCovered() const;

    QAudioDecoder *decoder = nullptr;
    QVector<Job> queue;
    QVector<Segment> segments;
    QVector<Accumulator> accs;
    QVector<double> scratch;
    QElapsedTimer progress_timer;
    std::atomic<bool> cancelled{false};
    bool running = false;
    bool job_active = false;
    bool abort_queued = false;
    bool format_known = false;
    int current = -1;
    int channels = 0;
    int sample_rate = 0;
    qint64 frame_pos = 0;
    qint64 finished_us = 0;
    qint64 total_us = 0;
  };
}

#endif // DYNAMIC_RANGE_ANALYZER_H
