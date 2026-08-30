#ifndef CLI_H
#define CLI_H

#include "ipc/instance.h"

#include <QStringList>

namespace Cli {
  QStringList arguments(int argc, char *argv[]);
  bool isVersionRequest(const QStringList &args);

  enum class Startup {
    Run,
    HandedOff,
    HandOffFailed
  };

  Startup claimInstance(IPC::Instance &instance, bool single_instance, const QStringList &args);
}

#endif // CLI_H
