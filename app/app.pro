QT       += core gui multimedia concurrent network
unix: QT += dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = mpz

DEFINES += TAGLIB_STATIC

win32: {
  CONFIG -= debug_and_release
  #CONFIG += static
  #QMAKE_LFLAGS += -static
}


include(../version.pri)

RC_ICONS = resources/icons/mpz.ico

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

INCLUDEPATH += . # without this lupdate shows errors: Qualifying with unknown namespace/class

SOURCES += \
    aboutdialog.cpp \
    busyspinner.cpp \
    config/global.cpp \
    config/local.cpp \
    config/storage.cpp \
    config/value.cpp \
    directory_ui/directorycontroller.cpp \
    directory_ui/directorysettings.cpp \
    feedback_ui/feedbackform.cpp \
    ipc/instance.cpp \
    main.cpp \
    mainmenu.cpp \
    mainwindow.cpp \
    directory_ui/directorymodel.cpp \
    playback/controls.cpp \
    playback/dispatch.cpp \
    playback/mediaplayer.cpp \
    playback/playbackcontroller.cpp \
    playback/randomtrail.cpp \
    playback_log_ui/playbacklogdialog.cpp \
    playback_log_ui/playbackloguicontroller.cpp \
    playback_log_ui/playbackloguimodel.cpp \
    playlist/cueparser.cpp \
    playlist/fileparser.cpp \
    playlist/loader.cpp \
    playlist/sorter.cpp \
    playlist_ui/columnsconfig.cpp \
    playlist_ui/playlistcontroller.cpp \
    playlist_ui/playlistmodel.cpp \
    playlist/playlist.cpp \
    playback/playerstate.cpp \
    playlist_ui/playlistproxyfiltermodel.cpp \
    playlist_ui/playlistcontextmenu.cpp \
    playlist_ui/trackinfodialog.cpp \
    playlists_ui/playlistscontextmenu.cpp \
    directory_ui/directorycontextmenu.cpp \
    playlists_ui/playlistscontroller.cpp \
    playlists_ui/playlistsmodel.cpp \
    playlists_ui/playlistsproxyfiltermodel.cpp \
    rnjesus.cpp \
    shortcuts.cpp \
    sort_ui/sortingpresetsdialog.cpp \
    sort_ui/sortmenu.cpp \
    statusbarlabel.cpp \
    sysinfo.cpp \
    track.cpp \
    trayicon.cpp \
    volumecontrol.cpp \
    playback/stream.cpp \
    streammetadata.cpp

HEADERS += \
    aboutdialog.h \
    busyspinner.h \
    config/global.h \
    config/local.h \
    config/storage.h \
    config/value.h \
    directory_ui/directorycontroller.h \
    directory_ui/directorysettings.h \
    feedback_ui/feedbackform.h \
    ipc/instance.h \
    mainmenu.h \
    mainwindow.h \
    directory_ui/directorymodel.h \
    playback/controls.h \
    playback/dispatch.h \
    playback/mediaplayer.h \
    playback/playbackcontroller.h \
    playback/randomtrail.h \
    playback_log_ui/playbacklogdialog.h \
    playback_log_ui/playbackloguicontroller.h \
    playback_log_ui/playbackloguimodel.h \
    playlist/cueparser.h \
    playlist/fileparser.h \
    playlist/loader.h \
    playlist/sorter.h \
    playlist_ui/columnsconfig.h \
    playlist_ui/playlistcontroller.h \
    playlist_ui/playlistmodel.h \
    playlist_ui/playlistcontextmenu.h \
    playlist_ui/trackinfodialog.h \
    playlists_ui/playlistscontextmenu.h \
    directory_ui/directorycontextmenu.h \
    playlist/playlist.h \
    playback/playerstate.h \
    playlist_ui/playlistproxyfiltermodel.h \
    playlists_ui/playlistscontroller.h \
    playlists_ui/playlistsmodel.h \
    playlists_ui/playlistsproxyfiltermodel.h \
    rnjesus.h \
    shortcuts.h \
    sort_ui/sortingpresetsdialog.h \
    sort_ui/sortmenu.h \
    statusbarlabel.h \
    sysinfo.h \
    track.h \
    trayicon.h \
    volumecontrol.h \
    playback/stream.h \
    streammetadata.h

FORMS += \
    aboutdialog.ui \
    directory_ui/directorysettings.ui \
    feedback_ui/feedbackform.ui \
    mainwindow.ui \
    playback_log_ui/playbacklogdialog.ui \
    playlist_ui/trackinfodialog.ui \
    sort_ui/sortingpresets.ui

# Libraries
INCLUDEPATH += \
  ../libs/taglib/taglib-1.12/taglib \
  ../libs/taglib/taglib-1.12/taglib/toolkit \
  ../libs/yaml-cpp/yaml-cpp-0.6.3/include \
  ../libs/qtwaitingspinner \
  ../libs/qhotkey/QHotkey-1.4.2

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

RESOURCES += \
  ../resources.qrc

# make install
target.path = /usr/bin/
INSTALLS += target
