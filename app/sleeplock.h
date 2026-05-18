#ifndef SHUTDOWNLOCK_H
#define SHUTDOWNLOCK_H

#include <QObject>
#include <QProcess>

#ifdef Q_OS_MACOS
  #include <IOKit/pwr_mgt/IOPMLib.h>
#endif

class SleepLock : public QObject {
  Q_OBJECT
public:
  explicit SleepLock(QObject *parent = nullptr);

public slots:
  void activate(bool state);

private:
  QProcess proc;
#ifdef Q_OS_MACOS
  IOPMAssertionID sleep_assertion = kIOPMNullAssertionID;
#endif
};

#endif // SHUTDOWNLOCK_H
