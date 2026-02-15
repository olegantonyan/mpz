cmake_minimum_required(VERSION 3.10.0)

project(mpz VERSION 2.0.4 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS Core Gui Widgets Multimedia Concurrent Network)

if(UNIX AND NOT APPLE)
  set(DEFAULT_ENABLE_DBUS ON)
else()
  set(DEFAULT_ENABLE_DBUS OFF)
endif()
option(ENABLE_DBUS "Enable DBus on Linux for MPRIS support" ${DEFAULT_ENABLE_DBUS})
if (ENABLE_DBUS)
  list(APPEND QT_COMPONENTS DBus)
  message(STATUS "DBus support ON")
else()
  message(STATUS "DBus support OFF")
endif()

option(USE_QT5 "Use Qt5, the default is Qt6" OFF)
if (USE_QT5)
  find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
  set(QT_VERSION_MAJOR ${Qt5Core_VERSION_MAJOR})
else()
  find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})
  set(QT_VERSION_MAJOR ${Qt6Core_VERSION_MAJOR})
endif()

set(PROJECT_SOURCES
  app/about_ui/aboutdialog.cpp
  app/busyspinner.cpp
  app/config/global.cpp
  app/config/local.cpp
  app/config/storage.cpp
  app/config/value.cpp
  app/coverart/embedded.cpp
  app/directory_ui/directorycontroller.cpp
  app/directory_ui/directorysettings.cpp
  app/feedback_ui/feedbackform.cpp
  app/ipc/instance.cpp
  app/main.cpp
  app/mainmenu.cpp
  app/mainwindow.cpp
  app/directory_ui/directorymodel/proxy.cpp
  app/directory_ui/directorymodel/localfs.cpp
  app/playback/controls.cpp
  app/playback/dispatch.cpp
  app/playback/mediaplayer.cpp
  app/playback/playbackcontroller.cpp
  app/playback/randomtrail.cpp
  app/playback_log_ui/playbacklogdialog.cpp
  app/playback_log_ui/playbackloguicontroller.cpp
  app/playback_log_ui/playbackloguimodel.cpp
  app/coverart/covers.cpp
  app/playlist/cueparser.cpp
  app/playlist/fileparser.cpp
  app/playlist/loader.cpp
  app/playlist/sorter.cpp
  app/playlist_ui/columnsconfig.cpp
  app/playlist_ui/playlistcontroller.cpp
  app/playlist_ui/playlistmodel.cpp
  app/playlist/playlist.cpp
  app/playback/playerstate.cpp
  app/playlist_ui/playlistproxyfiltermodel.cpp
  app/playlist_ui/playlistcontextmenu.cpp
  app/playlist_ui/trackinfodialog.cpp
  app/playlists_ui/playlistscontextmenu.cpp
  app/directory_ui/directorycontextmenu.cpp
  app/directory_ui/directorysortmenu.cpp
  app/playlists_ui/playlistscontroller.cpp
  app/playlists_ui/playlistsmodel.cpp
  app/playlists_ui/playlistsproxyfiltermodel.cpp
  app/rnjesus.cpp
  app/shortcuts.cpp
  app/shortcuts_ui/shortcutsdialog.cpp
  app/sleeplock.cpp
  app/sort_ui/sortingpresetsdialog.cpp
  app/sort_ui/sortmenu.cpp
  app/statusbarlabel.cpp
  app/sysinfo.cpp
  app/track.cpp
  app/trayicon.cpp
  app/volumecontrol.cpp
  app/playback/stream.cpp
  app/streammetadata.cpp
  app/modusoperandi.cpp
  app/about_ui/aboutdialog.ui
  app/directory_ui/directorysettings.ui
  app/feedback_ui/feedbackform.ui
  app/mainwindow.ui
  app/playback_log_ui/playbacklogdialog.ui
  app/playlist_ui/trackinfodialog.ui
  app/shortcuts_ui/shortcutsdialog.ui
  app/sort_ui/sortingpresets.ui
  resources.qrc
  app/slidingbanner.cpp
)

option(ENABLE_MPD_SUPPORT "Enable MPD support via libmpdclient" ON)
if (ENABLE_MPD_SUPPORT)
  list(APPEND PROJECT_SOURCES app/directory_ui/directorymodel/mpd.cpp)
  list(APPEND PROJECT_SOURCES app/directory_ui/directorymodel/mpd/treeitem.cpp)
  list(APPEND PROJECT_SOURCES app/playlists_ui/mpd/playlistsmodel.cpp)
  list(APPEND PROJECT_SOURCES app/playlist_ui/mpd/playlistmodel.cpp)
  list(APPEND PROJECT_SOURCES app/playlist/mpdloader.cpp)
  list(APPEND PROJECT_SOURCES app/playback/mpd/mediaplayer.cpp)
  list(APPEND PROJECT_SOURCES app/playback/mpd/playbackorder.cpp)
  list(APPEND PROJECT_SOURCES app/mpd_client/connection.cpp)
  list(APPEND PROJECT_SOURCES app/mpd_client/client.cpp)
  list(APPEND PROJECT_SOURCES app/mpd_client/entity.cpp)
  list(APPEND PROJECT_SOURCES app/mpd_client/status.cpp)
  list(APPEND PROJECT_SOURCES app/mpd_client/song.cpp)
  list(APPEND PROJECT_SOURCES app/mpd_client/output.cpp)
  list(APPEND PROJECT_SOURCES app/coverart/mpd.cpp)
  list(APPEND PROJECT_SOURCES app/directory_ui/addmpddialog.cpp)
endif()

if (ENABLE_DBUS)
  list(APPEND PROJECT_SOURCES app/dbus/mpris.cpp)
  if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.xml dbus/mpris.h Mpris)
    qt_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.Player.xml dbus/mpris.h Mpris)
  else()
    qt5_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.xml dbus/mpris.h Mpris)
    qt5_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.Player.xml dbus/mpris.h Mpris)
  endif()
endif()

if (WIN32)
  enable_language("RC")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
    set(CMAKE_RC_COMPILER_INIT windres)
    set(CMAKE_RC_FLAGS "--input-format=rc --output-format=coff")
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> --input-format=rc --output-format=coff -i <SOURCE> -o <OBJECT>")
  endif()
  list(APPEND PROJECT_SOURCES app/resources/icons/mpz.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  message(STATUS "using Qt6")
  list(APPEND PROJECT_SOURCES app/audio_device_ui/devicesmenu.cpp)
  qt_add_executable(${CMAKE_PROJECT_NAME} MANUAL_FINALIZATION ${PROJECT_SOURCES})
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_DEVICES_MENU)
else()
  message(STATUS "using Qt5")
  add_executable(${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES})
endif()

foreach(_qt_comp IN LISTS QT_COMPONENTS)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::${_qt_comp})
endforeach()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE VERSION="${PROJECT_VERSION}")
if (ENABLE_DBUS)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE MPRIS_ENABLE)
endif()
if (ENABLE_MPD_SUPPORT)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_MPD_SUPPORT)
endif()

set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER org.mpz-player.mpz)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
  ${BUNDLE_ID_OPTION}
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)












# 3RDPARTY
include(disable_installation.cmake)
set(_DISABLE_INSTALLATION TRUE)

# YAML-CPP
option(USE_SYSTEM_YAMLCPP "Use Yaml-cpp provided by the OS" OFF)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE yaml-cpp)
if(USE_SYSTEM_YAMLCPP)
  message(STATUS "using system-wide yaml-cpp")
else()
  set(YAMLCPP_DIR 3rdparty/yaml-cpp-0.9.0)
  message(STATUS "using vendored yaml-cpp from ${YAMLCPP_DIR}")
  add_definitions(-include stdint.h) # Fedora Rawhide broken without this headder (3rdparty/yaml-cpp-0.8.0/src/emitterutils.cpp:221:11: error: uint16_t was not declared in this scope)
  add_subdirectory(${YAMLCPP_DIR})
endif()
# ENDOF YAML-CPP

# TAGLIB
option(USE_SYSTEM_TAGLIB "Use Taglib provided by the OS" OFF)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE tag)
if (USE_SYSTEM_TAGLIB)
  message(STATUS "using system-wide taglib")
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_SYSTEM_TAGLIB)
else()
  set(TAGLIB_DIR 3rdparty/taglib-2.1.1)
  message(STATUS "using vendored taglib from ${TAGLIB_DIR}")
  add_subdirectory(${TAGLIB_DIR})
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
   ${TAGLIB_DIR}
   ${TAGLIB_DIR}/taglib
   ${TAGLIB_DIR}/taglib/ape
   ${TAGLIB_DIR}/taglib/asf
   ${TAGLIB_DIR}/taglib/dsdiff
   ${TAGLIB_DIR}/taglib/dsf
   ${TAGLIB_DIR}/taglib/flac
   ${TAGLIB_DIR}/taglib/it
   ${TAGLIB_DIR}/taglib/mod
   ${TAGLIB_DIR}/taglib/mp4
   ${TAGLIB_DIR}/taglib/mpeg
   ${TAGLIB_DIR}/taglib/mpeg/id3v1
   ${TAGLIB_DIR}/taglib/mpeg/id3v2
   ${TAGLIB_DIR}/taglib/mpeg/id3v2/frames
   ${TAGLIB_DIR}/taglib/ogg
   ${TAGLIB_DIR}/taglib/ogg/flac
   ${TAGLIB_DIR}/taglib/ogg/opus
   ${TAGLIB_DIR}/taglib/ogg/speex
   ${TAGLIB_DIR}/taglib/ogg/vorbis
   ${TAGLIB_DIR}/taglib/riff
   ${TAGLIB_DIR}/taglib/riff/aiff
   ${TAGLIB_DIR}/taglib/riff/wav
   ${TAGLIB_DIR}/taglib/s3m
   ${TAGLIB_DIR}/taglib/toolkit
   ${TAGLIB_DIR}/taglib/trueaudio
   ${TAGLIB_DIR}/taglib/wavpack
   ${TAGLIB_DIR}/taglib/xm
  )
endif()
# ENDOF TAGLIB

# QHOTKEY
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qhotkey)
set(QHOTKEY_DIR 3rdparty/QHotkey-1.5.0)
add_subdirectory(${QHOTKEY_DIR})
message(STATUS "using vendored qhotkey from ${QHOTKEY_DIR}")
# ENDOF QHOTKEY

# QTWAITINGSPINNER
set(QTWAITINGSPINNER_DIR 3rdparty/qtwaitingspinner)
add_library(qtwaitingspinner ${QTWAITINGSPINNER_DIR}/waitingspinnerwidget.cpp)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${QTWAITINGSPINNER_DIR})
target_link_libraries(qtwaitingspinner PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qtwaitingspinner)
# ENDOF QTWAITINGSPINNER

# LIBMPDCLIENT
if (ENABLE_MPD_SUPPORT)
  option(USE_SYSTEM_LIBMPDCLIENT "Use libmpdclient provided by the OS" OFF)
  if (USE_SYSTEM_LIBMPDCLIENT)
    message(STATUS "using system-wide libmpdclient")
    find_library(MPDCLIENT_LIB mpdclient REQUIRED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${MPDCLIENT_LIB})
  else()
    include(3rdparty/libmpdclient.cmake)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE mpdclient)
  endif()
endif()
# ENDOF LIBMPDCLIENT

# DEATHHANDLER
if (UNIX AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|arm64|x86|x86_64|amd64|i[3-6]86)$"))
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_DEATHHANDLER)
  set(DEATHHANDLER_DIR 3rdparty/DeathHandler)
  add_library(deathhandler ${DEATHHANDLER_DIR}/death_handler.cc)
  target_link_libraries(deathhandler PRIVATE dl)
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${DEATHHANDLER_DIR})
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE deathhandler)
endif()
# ENDOF DEATHHANDLER

set(_DISABLE_INSTALLATION FALSE)
# ENDOF 3RDPARTY













include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES mpz.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES license.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${CMAKE_PROJECT_NAME})
file(GLOB ICON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons/*x*/mpz.png")
foreach(icon IN LISTS ICON_FILES)
  get_filename_component(size_dir "${icon}" DIRECTORY)
  get_filename_component(size_dir "${size_dir}" NAME)
  install(FILES "${icon}" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size_dir}/apps)
endforeach()


if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(${CMAKE_PROJECT_NAME})
endif()
