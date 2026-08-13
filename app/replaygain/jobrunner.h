#ifndef REPLAYGAIN_JOBRUNNER_H
#define REPLAYGAIN_JOBRUNNER_H

#include "replaygain/scanjob.h"

#include <QObject>

namespace ReplayGain {
  class JobRunner : public QObject {
    Q_OBJECT
  public:
    explicit JobRunner(QObject *parent = nullptr);

  public slots:
    void run(const ReplayGain::Job &job);

  signals:
    void fileStarted(int epoch, const QString &path);
    void jobFinished(const ReplayGain::JobResult &result);
  };
}

#endif // REPLAYGAIN_JOBRUNNER_H
