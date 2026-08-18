#include "replaygain/scanworker.h"

#include "replaygain/jobcodec.h"
#include "replaygain/jobrunner.h"
#include "replaygain/scanjob.h"

#include <QCoreApplication>
#include <QFile>

#ifdef Q_OS_WIN
#  include <fcntl.h>
#  include <io.h>
#  include <stdio.h>
#endif

namespace ReplayGain {
  namespace {
    const char *kFlag = "--replaygain-worker";

    void useBinaryStdio() {
#ifdef Q_OS_WIN
      // the framing is binary; Windows would otherwise rewrite every 0x0A
      _setmode(_fileno(stdin), _O_BINARY);
      _setmode(_fileno(stdout), _O_BINARY);
#endif
    }
  }

  QString scanWorkerFlag() {
    return QString::fromLatin1(kFlag);
  }

  bool isScanWorkerInvocation(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
      if (qstrcmp(argv[i], kFlag) == 0) {
        return true;
      }
    }
    return false;
  }

  int runScanWorker(int argc, char *argv[]) {
    useBinaryStdio();
    QCoreApplication app(argc, argv);

    QFile input;
    if (!input.open(stdin, QIODevice::ReadOnly)) {
      return 2;
    }
    Job job;
    if (!decodeJob(input.readAll(), job)) {
      return 2;
    }

    QFile output;
    if (!output.open(stdout, QIODevice::WriteOnly)) {
      return 2;
    }

    JobRunner runner;
    QObject::connect(&runner, &JobRunner::fileStarted, &app,
                     [&output](int, const QString &path) {
                       output.write(frameFileStarted(path));
                       output.flush();
                     });
    QObject::connect(&runner, &JobRunner::jobFinished, &app,
                     [&output](const ReplayGain::JobResult &result) {
                       output.write(frameJobDone(result));
                       output.flush();
                     });
    runner.run(job);
    output.flush();
    return 0;
  }
}
