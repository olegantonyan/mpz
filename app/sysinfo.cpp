#include "sysinfo.h"

#include <QSysInfo>
#include <QApplication>

QStringList SysInfo::get() {
  QStringList si;
  si << "App version: " + QString(VERSION);
  si << "Qt version: " + QString(qVersion());
  si << "Build ABI: " + QSysInfo::buildAbi();
  si << "Build CPU architecture: " + QSysInfo::buildCpuArchitecture();
  si << "Current CPU architecture: " + QSysInfo::currentCpuArchitecture();
  si << "Kernel type: " + QSysInfo::kernelType();
  si << "Kernel version: " + QSysInfo::kernelVersion();
  si << "Product name: " + QSysInfo::prettyProductName();
  return si;
}
