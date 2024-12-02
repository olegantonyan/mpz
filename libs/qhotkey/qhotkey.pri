mac: LIBS += -framework Carbon
else:win32: LIBS += -luser32
else:unix {
  lessThan(QT_MAJOR_VERSION, 6): QT += x11extras
  LIBS += -lX11
}
