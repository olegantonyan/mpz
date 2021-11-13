TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++11

TARGET = qhotkey

win32: CONFIG -= debug_and_release

HEADERS += \
  QHotkey-1.5.0/QHotkey/qhotkey.h \
  QHotkey-1.5.0/QHotkey/QHotkey \
  QHotkey-1.5.0/QHotkey/qhotkey_p.h

SOURCES += \
  QHotkey-1.5.0/QHotkey/qhotkey.cpp

mac:        SOURCES += QHotkey-1.5.0/QHotkey/qhotkey_mac.cpp
else:win32: SOURCES += QHotkey-1.5.0/QHotkey/qhotkey_win.cpp
else:unix:  SOURCES += QHotkey-1.5.0/QHotkey/qhotkey_x11.cpp

INCLUDEPATH += QHotkey-1.5.0/QHotkey

include(qhotkey.pri)

DEFINES += QHOTKEY_LIB QHOTKEY_LIB_BUILD


