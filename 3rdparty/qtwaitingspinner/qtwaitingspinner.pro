QT += widgets

TEMPLATE = lib
CONFIG += staticlib
CONFIG += c++17

win32: CONFIG -= debug_and_release

TARGET = qtwaitingspinner

SOURCES += \
    waitingspinnerwidget.cpp
    
HEADERS += \
    waitingspinnerwidget.h
