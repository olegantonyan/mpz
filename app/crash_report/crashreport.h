#ifndef CRASHREPORT_H
#define CRASHREPORT_H

#include <QString>

struct CrashEntry {
  bool valid = false;
  QString id;
  QString text;
};

CrashEntry lastCrash(const QString &log);

#endif // CRASHREPORT_H
