#include "sleeplock.h"

#if defined(Q_OS_LINUX) && defined(MPZ_ENABLE_DBUS)
  #include <QDBusConnection>
  #include <QDBusMessage>
  #include <QDBusReply>
  #include <QDBusObjectPath>
  #include <QVariant>
#elif defined(Q_OS_LINUX)
  #include <QStringList>
  #include <QApplication>
#endif

#ifdef Q_OS_WIN
  #include <windows.h>
#endif

SleepLock::SleepLock(QObject *parent) : QObject(parent)
#if defined(Q_OS_LINUX) && !defined(MPZ_ENABLE_DBUS)
  , proc(parent)
#endif
{
#if defined(Q_OS_LINUX) && !defined(MPZ_ENABLE_DBUS)
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

void SleepLock::activate(bool state) {
#if defined(Q_OS_LINUX) && defined(MPZ_ENABLE_DBUS)
  if (state) {
    if (inhibit_handle.isEmpty()) {
      QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Inhibit"),
        QStringLiteral("Inhibit"));
      msg << QString()
          << uint(12) // suspend | idle
          << QVariantMap{ { QStringLiteral("reason"), QStringLiteral("playing music") } };
      QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);
      if (reply.isValid()) {
        inhibit_handle = reply.value().path();
      }
    }
  } else {
    if (!inhibit_handle.isEmpty()) {
      QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        inhibit_handle,
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Close"));
      QDBusConnection::sessionBus().call(msg);
      inhibit_handle.clear();
    }
  }
#elif defined(Q_OS_LINUX)
  try {
    if (state) {
      if (proc.state() == QProcess::NotRunning) {
        proc.start();
      }
    } else {
      proc.kill();
    }
  } catch (...) {
  }
#elif defined(Q_OS_WIN)
  if (state) {
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
  } else {
    SetThreadExecutionState(ES_CONTINUOUS);
  }
#elif defined(Q_OS_MACOS)
  if (state) {
    if (sleep_assertion == kIOPMNullAssertionID) {
      CFStringRef reason = CFSTR("mpz is playing music");
      IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep,
                                  kIOPMAssertionLevelOn, reason, &sleep_assertion);
    }
  } else {
    if (sleep_assertion != kIOPMNullAssertionID) {
      IOPMAssertionRelease(sleep_assertion);
      sleep_assertion = kIOPMNullAssertionID;
    }
  }
#else
  Q_UNUSED(state)
#endif
}
