#include "replaygain/scanner.h"

#include "replaygain/jobrunner.h"

#include <QThread>

namespace ReplayGain {
  namespace {
    const int kMaxWorkers = 4;
    const int kProgressIntervalMs = 100;
  }

  Scanner::Scanner(QObject *parent) : QObject(parent) {
    qRegisterMetaType<ReplayGain::Job>();
    qRegisterMetaType<ReplayGain::SliceResult>();
    qRegisterMetaType<ReplayGain::JobResult>();
  }

  Scanner::~Scanner() {
    cancel();
    for (auto *thread : threads) {
      thread->quit();
      thread->wait();
      delete thread;
    }
  }

  int Scanner::defaultWorkerCount() {
    // A starved GUI thread underruns the engine's sink, so leave a core free.
    return qBound(1, QThread::idealThreadCount() - 1, kMaxWorkers);
  }

  void Scanner::ensureWorkers(int count) {
    while (threads.size() < count) {
      auto *thread = new QThread;
      auto *runner = new JobRunner;
      runner->moveToThread(thread);
      connect(thread, &QThread::finished, runner, &QObject::deleteLater);
      connect(runner, &JobRunner::jobFinished, this, &Scanner::onJobFinished);
      connect(runner, &JobRunner::fileStarted, this, [this](int job_epoch, const QString &path) {
        if (job_epoch == epoch) {
          emitProgress(path, false);
        }
      });
      thread->start();

      threads.append(thread);
      runners.append(runner);
      busy.append(false);
    }
  }

  void Scanner::start(const QVector<Job> &jobs, int worker_count) {
    open(worker_count);
    enqueue(jobs);
    producerFinished();
  }

  void Scanner::open(int worker_count) {
    cancel();

    epoch++;
    cancelling = false;
    producer_done = false;
    workers_wanted = worker_count > 0 ? worker_count : defaultWorkerCount();
    abort = std::make_shared<std::atomic<bool>>(false);
    total_slices = 0;
    done_slices = 0;
    analysed = 0;
    failed = 0;
    throttle.invalidate();
  }

  void Scanner::enqueue(const QVector<Job> &jobs) {
    if (producer_done) {
      return;
    }

    for (Job job : jobs) {
      if (job.files.isEmpty()) {
        continue;
      }
      job.epoch = epoch;
      job.abort = abort;
      total_slices += job.sliceCount();
      pending.enqueue(job);
    }

    if (pending.isEmpty()) {
      return;
    }
    ensureWorkers(qMin(workers_wanted, pending.size()));
    emitProgress(QString(), true);
    dispatch();
  }

  void Scanner::producerFinished() {
    if (producer_done) {
      return;
    }
    producer_done = true;
    if (in_flight == 0 && pending.isEmpty()) {
      emitProgress(QString(), true);
      emit finished(analysed, failed, cancelling);
      cancelling = false;
    }
  }

  void Scanner::dispatch() {
    for (int i = 0; i < runners.size() && !pending.isEmpty(); i++) {
      if (busy.at(i)) {
        continue;
      }
      busy[i] = true;
      in_flight++;
      QMetaObject::invokeMethod(runners.at(i), "run", Qt::QueuedConnection,
                                Q_ARG(ReplayGain::Job, pending.dequeue()));
    }
  }

  void Scanner::onJobFinished(const ReplayGain::JobResult &result) {
    auto *runner = qobject_cast<JobRunner *>(sender());
    const int index = runners.indexOf(runner);
    if (index >= 0) {
      busy[index] = false;
    }
    in_flight--;

    if (result.epoch == epoch) {
      for (const auto &slice : result.slices) {
        done_slices++;
        if (slice.ok) {
          analysed++;
        } else {
          failed++;
        }
        emit sliceAnalyzed(slice);
      }
      emitProgress(QString(), false);
      dispatch();
    }

    if (in_flight == 0 && pending.isEmpty() && producer_done) {
      emitProgress(QString(), true);
      emit finished(analysed, failed, cancelling);
      cancelling = false;
    }
  }

  void Scanner::emitProgress(const QString &path, bool force) {
    if (!force && throttle.isValid() && throttle.elapsed() < kProgressIntervalMs) {
      return;
    }
    throttle.start();
    emit progress(done_slices, total_slices, path);
  }

  void Scanner::cancel() {
    if (!isScanning()) {
      return;
    }
    cancelling = true;
    epoch++;
    pending.clear();
    if (abort) {
      abort->store(true);
    }
    producer_done = false;
    producerFinished();
  }
}
