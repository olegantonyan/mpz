#ifndef REPLAYGAIN_SCANNER_H
#define REPLAYGAIN_SCANNER_H

#include "replaygain/scanjob.h"
#include "replaygain/jobrunner.h"

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QThread>
#include <QVector>

namespace ReplayGain {
  class Scanner : public QObject {
    Q_OBJECT
  public:
    explicit Scanner(QObject *parent = nullptr);
    ~Scanner() override;

    static int defaultWorkerCount();

    bool isScanning() const { return in_flight > 0 || !pending.isEmpty() || !producer_done; }

    void start(const QVector<Job> &jobs, int worker_count = 0);

    // Incremental form: open(), then enqueue() as jobs are discovered, then
    // producerFinished(). finished() is held back until the producer is done.
    void open(int worker_count = 0);
    void enqueue(const QVector<Job> &jobs);
    void producerFinished();

    void cancel();

  signals:
    void progress(int done, int total, const QString &path);
    void sliceAnalyzed(const ReplayGain::SliceResult &result);
    void finished(int analysed, int failed, bool cancelled);

  private:
    void ensureWorkers(int count);
    void dispatch();
    void onJobFinished(const ReplayGain::JobResult &result);
    void emitProgress(const QString &path, bool force);

    QVector<QThread *> threads;
    QVector<JobRunner *> runners;
    QVector<bool> busy;

    QQueue<Job> pending;
    QElapsedTimer throttle;
    std::shared_ptr<std::atomic<bool>> abort;

    int epoch = 0;
    int in_flight = 0;
    int total_slices = 0;
    int done_slices = 0;
    int analysed = 0;
    int failed = 0;
    int workers_wanted = 0;
    bool cancelling = false;
    bool producer_done = true;
  };
}

#endif // REPLAYGAIN_SCANNER_H
