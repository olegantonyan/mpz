#ifndef REPLAYGAIN_JOBRUNNER_H
#define REPLAYGAIN_JOBRUNNER_H

#include "replaygain/scanjob.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace ReplayGain {
  class JobRunner : public QObject {
    Q_OBJECT
  public:
    explicit JobRunner(QObject *parent = nullptr);

    // With a worker set, decoding happens in a child process: a malformed file can
    // segfault inside Qt's ffmpeg plugin, on a thread nothing here can catch. Unset
    // (the default) decodes in this process.
    void setWorker(const QString &program, const QStringList &arguments);

  public slots:
    void run(const ReplayGain::Job &job);

  signals:
    void fileStarted(int epoch, const QString &path);
    void jobFinished(const ReplayGain::JobResult &result);

  private:
    void runInProcess(const ReplayGain::Job &job);
    bool runIsolated(const ReplayGain::Job &job);

    QString worker_program;
    QStringList worker_arguments;
  };
}

#endif // REPLAYGAIN_JOBRUNNER_H
