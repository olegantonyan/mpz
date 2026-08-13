#ifndef SYSINFO_H
#define SYSINFO_H

#include <QStringList>

class SysInfo {
public:
  static QStringList get();
  static bool sandboxed();
};

#endif // SYSINFO_H
