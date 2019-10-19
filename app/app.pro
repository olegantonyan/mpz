QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

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
    config/global.cpp \
    config/local.cpp \
    config/storage.cpp \
    config/value.cpp \
    main.cpp \
    mainwindow.cpp \
    directory_ui/directorymodel.cpp \
    directory_ui/directoryview.cpp \
    playback/controls.cpp \
    playback/playbackview.cpp \
    playlist_ui/playlistmodel.cpp \
    playlist_ui/playlistview.cpp \
    playlist.cpp \
    playlists_ui/playlistsmodel.cpp \
    playlists_ui/playlistsview.cpp \
    track.cpp

HEADERS += \
    config/global.h \
    config/local.h \
    config/storage.h \
    config/value.h \
    mainwindow.h \
    directory_ui/directorymodel.h \
    directory_ui/directoryview.h \
    playback/controls.h \
    playback/playbackview.h \
    playlist_ui/playlistmodel.h \
    playlist_ui/playlistview.h \
    playlist.h \
    playlists_ui/playlistsmodel.h \
    playlists_ui/playlistsview.h \
    track.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += \
  ../libs/taglib/taglib-1.11.1/taglib \
  ../libs/taglib/taglib-1.11.1/taglib/toolkit \
  ../libs/yaml-cpp/yaml-cpp-0.6.2/include \

LIBS += \
  -L../libs/taglib -ltaglib \
  -L../libs/yaml-cpp -lyaml-cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
  resources/resources.qrc

