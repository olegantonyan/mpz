#ifndef DYNAMIC_RANGE_SCANNER_H
#define DYNAMIC_RANGE_SCANNER_H

#include "dynamic_range/analyzer.h"

#include <QObject>
#include <QThread>

namespace DynamicRange {
  class Scanner : public QObject {
    Q_OBJECT

  public:
    explicit Scanner(QObject *parent = nullptr);
    ~Scanner();

    void run(const QVector<DynamicRange::Job> &jobs);
    void cancel();

  signals:
    void progress(qint64 done_us, qint64 total_us);
    void segmentDone(quint64 uid, const DynamicRange::Result &result);
    void finished(bool cancelled);

  private:
    QThread *thread;
    Analyzer *analyzer;
  };
}

#endif // DYNAMIC_RANGE_SCANNER_H
