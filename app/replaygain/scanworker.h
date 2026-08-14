#ifndef REPLAYGAIN_SCANWORKER_H
#define REPLAYGAIN_SCANWORKER_H

#include <QString>
#include <QStringList>

namespace ReplayGain {
  QString scanWorkerFlag();
  bool isScanWorkerInvocation(int argc, char *argv[]);

  // Reads one job from stdin, writes framed progress and the result to stdout.
  // No GUI and no crash handler: the parent reads a crash exit as the answer.
  int runScanWorker(int argc, char *argv[]);
}

#endif // REPLAYGAIN_SCANWORKER_H
