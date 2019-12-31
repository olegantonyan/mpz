QT       += core gui multimedia concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = mpz

# QMAKE_LFLAGS += -static

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    busyspinner.cpp \
    config/global.cpp \
    config/local.cpp \
    config/storage.cpp \
    config/value.cpp \
    directory_ui/directorycontroller.cpp \
    directory_ui/directorysettings.cpp \
    main.cpp \
    mainwindow.cpp \
    directory_ui/directorymodel.cpp \
    playback/controls.cpp \
    playback/dispatch.cpp \
    playback/playbackcontroller.cpp \
    playback/randomtrail.cpp \
    playlist_ui/playlistcontroller.cpp \
    playlist_ui/playlistmodel.cpp \
    playlist.cpp \
    playerstate.cpp \
    playlists_ui/playlistscontroller.cpp \
    playlists_ui/playlistsmodel.cpp \
    statusbarlabel.cpp \
    track.cpp \
    trayicon.cpp \
    volumemenu.cpp

HEADERS += \
    busyspinner.h \
    config/global.h \
    config/local.h \
    config/storage.h \
    config/value.h \
    directory_ui/directorycontroller.h \
    directory_ui/directorysettings.h \
    mainwindow.h \
    directory_ui/directorymodel.h \
    playback/controls.h \
    playback/dispatch.h \
    playback/playbackcontroller.h \
    playback/randomtrail.h \
    playlist_ui/playlistcontroller.h \
    playlist_ui/playlistmodel.h \
    playlist.h \
    playerstate.h \
    playlists_ui/playlistscontroller.h \
    playlists_ui/playlistsmodel.h \
    statusbarlabel.h \
    track.h \
    trayicon.h \
    volumemenu.h

FORMS += \
    directory_ui/directorysettings.ui \
    mainwindow.ui

INCLUDEPATH += \
  ../libs/taglib/taglib-1.11.1/taglib \
  ../libs/taglib/taglib-1.11.1/taglib/toolkit \
  ../libs/yaml-cpp/yaml-cpp-0.6.2/include \
  ../libs/qtwaitingspinner \

LIBS += \
  -L../libs/taglib -ltaglib \
  -L../libs/yaml-cpp -lyaml-cpp \
  -L../libs/qtwaitingspinner -lqtwaitingspinner

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
  resources/resources.qrc

