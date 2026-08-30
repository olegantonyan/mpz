#include "cli.h"

#include <QDebug>

namespace Cli {
  QStringList arguments(int argc, char *argv[]) {
    QStringList result;
    for (int i = 1; i < argc; i++) {
      result << argv[i];
    }
    return result;
  }

  bool isVersionRequest(const QStringList &args) {
    return args.size() == 1 && args.first() == "--version";
  }

  Startup claimInstance(IPC::Instance &instance, bool single_instance, const QStringList &args) {
    if (!single_instance) {
      return Startup::Run;
    }
    const int another_pid = instance.anotherPid();
    if (another_pid > 0) {
      qDebug() << "reusing another instance with pid" << another_pid;
      return instance.load_files_send(args) ? Startup::HandedOff : Startup::HandOffFailed;
    }
    instance.start();
    return Startup::Run;
  }
}
