#ifndef SHUTDOWNLOCK_H
#define SHUTDOWNLOCK_H

#include <QObject>
#include <QString>

#if defined(Q_OS_LINUX) && !defined(MPZ_ENABLE_DBUS)
  #include <QProcess>
#endif

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
#if defined(Q_OS_LINUX) && defined(MPZ_ENABLE_DBUS)
  QString inhibit_handle;
#elif defined(Q_OS_LINUX)
  QProcess proc;
#endif
#ifdef Q_OS_MACOS
  IOPMAssertionID sleep_assertion = kIOPMNullAssertionID;
#endif
};

#endif // SHUTDOWNLOCK_H
