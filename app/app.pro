QT       += core gui multimedia concurrent network
unix: QT += dbus

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

unix: DEFINES += MPRIS_ENABLE

SOURCES += \
    aboutdialog.cpp \
    busyspinner.cpp \
    config/global.cpp \
    config/local.cpp \
    config/storage.cpp \
    config/value.cpp \
    directory_ui/directorycontroller.cpp \
    directory_ui/directorysettings.cpp \
    main.cpp \
    mainmenu.cpp \
    mainwindow.cpp \
    directory_ui/directorymodel.cpp \
    playback/controls.cpp \
    playback/dispatch.cpp \
    playback/mediaplayer.cpp \
    playback/playbackcontroller.cpp \
    playback/randomtrail.cpp \
    playlist/fileparser.cpp \
    playlist_ui/playlistcontroller.cpp \
    playlist_ui/playlistmodel.cpp \
    playlist/playlist.cpp \
    playback/playerstate.cpp \
    playlist_ui/playlistproxyfiltermodel.cpp \
    playlist_ui/playlistcontextmenu.cpp \
    playlists_ui/playlistscontextmenu.cpp \
    directory_ui/directorycontextmenu.cpp \
    playlists_ui/playlistscontroller.cpp \
    playlists_ui/playlistsmodel.cpp \
    playlists_ui/playlistsproxyfiltermodel.cpp \
    statusbarlabel.cpp \
    track.cpp \
    trayicon.cpp \
    volumecontrol.cpp \
    playback/stream.cpp \
    playback/streammetadata.cpp

HEADERS += \
    aboutdialog.h \
    busyspinner.h \
    config/global.h \
    config/local.h \
    config/storage.h \
    config/value.h \
    directory_ui/directorycontroller.h \
    directory_ui/directorysettings.h \
    mainmenu.h \
    mainwindow.h \
    directory_ui/directorymodel.h \
    playback/controls.h \
    playback/dispatch.h \
    playback/mediaplayer.h \
    playback/playbackcontroller.h \
    playback/randomtrail.h \
    playlist/fileparser.h \
    playlist_ui/playlistcontroller.h \
    playlist_ui/playlistmodel.h \
    playlist_ui/playlistcontextmenu.h \
    playlists_ui/playlistscontextmenu.h \
    directory_ui/directorycontextmenu.h \
    playlist/playlist.h \
    playback/playerstate.h \
    playlist_ui/playlistproxyfiltermodel.h \
    playlists_ui/playlistscontroller.h \
    playlists_ui/playlistsmodel.h \
    playlists_ui/playlistsproxyfiltermodel.h \
    statusbarlabel.h \
    track.h \
    trayicon.h \
    volumecontrol.h \
    playback/stream.h \
    playback/streammetadata.h

FORMS += \
    aboutdialog.ui \
    directory_ui/directorysettings.ui \
    mainwindow.ui

# Libraries
INCLUDEPATH += \
  ../libs/taglib/taglib-1.11.1/taglib \
  ../libs/taglib/taglib-1.11.1/taglib/toolkit \
  ../libs/yaml-cpp/yaml-cpp-0.6.2/include \
  ../libs/qtwaitingspinner \
  ../libs/qhotkey/QHotkey-1.2.2

LIBS += \
  -L../libs/taglib -ltaglib \
  -L../libs/yaml-cpp -lyaml-cpp \
  -L../libs/qtwaitingspinner -lqtwaitingspinner \
  -L../libs/qhotkey -lqhotkey

include(../libs/qhotkey/qhotkey.pri)
# End of libraries

unix: {
  DBUS_ADAPTORS += \
    dbus/org.mpris.MediaPlayer2.xml \
    dbus/org.mpris.MediaPlayer2.Player.xml

  HEADERS += \
    dbus/mpris.h

  SOURCES += \
    dbus/mpris.cpp
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
  resources/resources.qrc

