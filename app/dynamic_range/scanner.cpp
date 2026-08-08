#include "dynamic_range/scanner.h"

namespace DynamicRange {
  Scanner::Scanner(QObject *parent) : QObject(parent) {
    thread = new QThread;
    analyzer = new Analyzer();
    connect(analyzer, &Analyzer::progress, this, &Scanner::progress);
    connect(analyzer, &Analyzer::segmentDone, this, &Scanner::segmentDone);
    connect(analyzer, &Analyzer::finished, this, &Scanner::finished);
    analyzer->moveToThread(thread);
    connect(thread, &QThread::finished, analyzer, &QObject::deleteLater);
    thread->start();
  }

  Scanner::~Scanner() {
    analyzer->cancel();
    thread->quit();
    if (thread->wait(3000)) {
      thread->deleteLater();
      return;
    }
    // a decoder stuck probing an unreachable file must not hang application shutdown
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  }

  void Scanner::run(const QVector<Job> &jobs) {
    analyzer->run(jobs);
  }

  void Scanner::cancel() {
    analyzer->cancel();
  }
}
