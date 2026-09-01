#ifndef ASYNCTASKS_H
#define ASYNCTASKS_H

#include <QFuture>
#include <QList>
#include <QThreadPool>
#include <QtConcurrent>

#include <algorithm>

class AsyncTasks {
public:
  static AsyncTasks &instance() {
    static AsyncTasks self;
    return self;
  }

  template <typename F>
  QFuture<void> run(F fn) {
    forget_finished();
    QFuture<void> future = QtConcurrent::run(QThreadPool::globalInstance(), fn);
    pending.append(future);
    return future;
  }

  void drain() {
    const auto snapshot = pending;
    pending.clear();
    for (const auto &future : snapshot) {
      QFuture<void> f = future;
      f.waitForFinished();
    }
  }

private:
  void forget_finished() {
    pending.erase(std::remove_if(pending.begin(), pending.end(),
                                 [](const QFuture<void> &f) { return f.isFinished(); }),
                  pending.end());
  }

  QList<QFuture<void>> pending;
};

#endif // ASYNCTASKS_H
