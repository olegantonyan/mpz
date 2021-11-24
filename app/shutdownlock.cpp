#include "shutdownlock.h"

#include <QStringList>
#include <QApplication>
#include <QDebug>

ShutdownLock::ShutdownLock(QObject *parent) : QObject(parent), proc(parent) {
#ifdef Q_OS_LINUX
  proc.setProgram("systemd-inhibit");
  QStringList args;
  args << "--what=sleep:idle";
  args << QString("--who=%1").arg(qApp->applicationName());
  args << "--mode=block";
  args << "--why=playing music";
  args << "cat";
  proc.setArguments(args);
#endif
}

void ShutdownLock::activate(bool state) {
#ifdef Q_OS_LINUX
  try {
    if (state) {
      proc.start();
      /*
      connect(&proc, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qDebug() << error;
      });
      connect(&proc, &QProcess::started, [=]() {
        qDebug() << "started" << proc.processId();
      });
      connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "finished" << exitCode << exitStatus;
      });
      */
    } else {
      proc.kill();
    }
  }  catch (...) {
    qDebug() << "error starting/stopping sleep lock";
  }
#endif
}
