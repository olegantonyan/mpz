#ifndef REPLAYGAIN_SCANNER_H
#define REPLAYGAIN_SCANNER_H

#include "replaygain/scanjob.h"

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QVector>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace ReplayGain {
  class JobRunner;

  class Scanner : public QObject {
    Q_OBJECT
  public:
    explicit Scanner(QObject *parent = nullptr);
    ~Scanner() override;

    static int defaultWorkerCount();

    bool isScanning() const { return in_flight > 0 || !pending.isEmpty(); }

    void start(const QVector<Job> &jobs, int worker_count = 0);
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

    int epoch = 0;
    int in_flight = 0;
    int total_slices = 0;
    int done_slices = 0;
    int analysed = 0;
    int failed = 0;
    bool cancelling = false;
  };
}

#endif // REPLAYGAIN_SCANNER_H
