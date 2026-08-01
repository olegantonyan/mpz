#ifndef REPLAYGAIN_JOBRUNNER_H
#define REPLAYGAIN_JOBRUNNER_H

#include "replaygain/scanjob.h"

#include <QObject>

#include <atomic>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

namespace ReplayGain {
  class JobRunner : public QObject {
    Q_OBJECT
  public:
    explicit JobRunner(QObject *parent = nullptr);

  public slots:
    void run(const ReplayGain::Job &job);
    void cancel();

  signals:
    void fileStarted(int epoch, const QString &path);
    void jobFinished(const ReplayGain::JobResult &result);

  private:
    std::atomic<bool> cancelled{false};
    QEventLoop *active_loop = nullptr;
  };
}

#endif // REPLAYGAIN_JOBRUNNER_H
