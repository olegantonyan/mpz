cmake_minimum_required(VERSION 3.10.0)

project(mpz VERSION 1.1.4 LANGUAGES CXX)
ADD_DEFINITIONS(-DVERSION="${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

OPTION(USE_QT5, "Use Qt5, the default is Qt6" OFF)
if (USE_QT5)
  find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core)
else()
  find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
if (UNIX AND NOT APPLE)
  set(ENABLE_DBUS True)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus)
endif()


set(PROJECT_SOURCES
  app/about_ui/aboutdialog.cpp
  app/audio_device_ui/devicesmenu.cpp
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
  app/directory_ui/directorymodel.cpp
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
  app/about_ui/aboutdialog.ui
  app/directory_ui/directorysettings.ui
  app/feedback_ui/feedbackform.ui
  app/mainwindow.ui
  app/playback_log_ui/playbacklogdialog.ui
  app/playlist_ui/trackinfodialog.ui
  app/shortcuts_ui/shortcutsdialog.ui
  app/sort_ui/sortingpresets.ui
  resources.qrc
)

if (ENABLE_DBUS)
  ADD_DEFINITIONS(-DMPRIS_ENABLE)
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
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> --input-format=rc --output-format=coff -i <SOURCE> -o <OBJECT>"
    )
  endif()
  list(APPEND PROJECT_SOURCES app/resources/icons/mpz.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${CMAKE_PROJECT_NAME} MANUAL_FINALIZATION ${PROJECT_SOURCES})
  message(STATUS "using Qt6")
else()
  add_executable(${CMAKE_PROJECT_NAME} ${PROJECT_SOURCES})
  message(STATUS "using Qt5")
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Multimedia)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)
if (ENABLE_DBUS)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

add_definitions(-include stdint.h) # Fedora Rawhide broken without this headder (3rdparty/yaml-cpp-0.8.0/src/emitterutils.cpp:221:11: error: uint16_t was not declared in this scope)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER org.mpz-player.mpz)
endif()
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)












# 3RDPATY
if(NOT DEFINED _DISABLE_INSTALLATION)
  # This variable is responsible for installation disabling.
  set(_DISABLE_INSTALLATION FALSE)
  # Replace install() with conditional installation.
  macro(install)
    if (NOT _DISABLE_INSTALLATION)
      _install(${ARGN})
    endif()
  endmacro()
endif()
set(_DISABLE_INSTALLATION TRUE)

# YAML-CPP
OPTION(USE_SYSTEM_YAMLCPP, "Use Yaml-cpp provided by the OS" OFF)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE yaml-cpp)
if(USE_SYSTEM_YAMLCPP)
  message(STATUS "using system-wide yaml-cpp")
else()
  set(YAMLCPP_DIR 3rdparty/yaml-cpp-0.8.0)
  message(STATUS "using vendored yaml-cpp from ${YAMLCPP_DIR}")
  add_subdirectory(${YAMLCPP_DIR})
endif()
# ENDOF YAML-CPP

# TAGLIB
OPTION(USE_SYSTEM_TAGLIB "Use Taglib provided by the OS" OFF)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE tag)
if (USE_SYSTEM_TAGLIB)
  message(STATUS "using system-wide taglib")
  ADD_DEFINITIONS(-DUSE_SYSTEM_TAGLIB)
else()
  set(utf8cpp_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/utfcpp-4.0.6/source) # dependency required by taglib
  set(TAGLIB_DIR 3rdparty/taglib-2.1.1)
  message(STATUS "using vendored taglib from ${TAGLIB_DIR}")
  ADD_DEFINITIONS(-DTAGLIB_STATIC)
  set(BUILD_TESTING OFF)
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
OPTION(ENABLE_MPD_SUPPORT "Enable MPD support via libmpdclient" ON)
if (ENABLE_MPD_SUPPORT)
  OPTION(USE_SYSTEM_LIBMPDCLIENT "Use libmpdclient provided by the OS" OFF)
  if (USE_SYSTEM_LIBMPDCLIENT)
    message(STATUS "using system-wide libmpdclient")
    find_library(MPDCLIENT_LIB mpdclient REQUIRED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${MPDCLIENT_LIB})
  else()
    set(LIBMPDCLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libmpdclient-2.23)
    message(STATUS "using vendored libmpdclient from ${LIBMPDCLIENT_DIR}")
    set(LIBMPDCLIENT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/libmpdclient)
    add_custom_command(
        OUTPUT ${LIBMPDCLIENT_BUILD_DIR}/libmpdclient.a
        COMMAND meson setup ${LIBMPDCLIENT_BUILD_DIR} ${LIBMPDCLIENT_DIR} --default-library=static
        COMMAND ninja -C ${LIBMPDCLIENT_BUILD_DIR}
        DEPENDS ${LIBMPDCLIENT_DIR}/meson.build
        COMMENT "Building libmpdclient with Meson"
    )
    add_custom_target(build_libmpdclient DEPENDS ${LIBMPDCLIENT_BUILD_DIR}/libmpdclient.a)
    add_dependencies(${CMAKE_PROJECT_NAME} build_libmpdclient)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${LIBMPDCLIENT_DIR}/include)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${LIBMPDCLIENT_BUILD_DIR}/libmpdclient.a)
  endif()
endif()
# ENDOF LIBMPDCLIENT

set(_DISABLE_INSTALLATION FALSE)
# ENDOF 3RDPATY













include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES mpz.desktop DESTINATION share/applications)
install(FILES app/resources/icons/512x512/mpz.png DESTINATION share/icons/hicolor/512x512/apps/)
install(FILES app/resources/icons/256x256/mpz.png DESTINATION share/icons/hicolor/256x256/apps/)
install(FILES app/resources/icons/192x192/mpz.png DESTINATION share/icons/hicolor/192x192/apps/)
install(FILES app/resources/icons/128x128/mpz.png DESTINATION share/icons/hicolor/128x128/apps/)
install(FILES app/resources/icons/96x96/mpz.png DESTINATION share/icons/hicolor/96x96/apps/)
install(FILES app/resources/icons/72x72/mpz.png DESTINATION share/icons/hicolor/72x72/apps/)
install(FILES app/resources/icons/64x64/mpz.png DESTINATION share/icons/hicolor/64x64/apps/)
install(FILES app/resources/icons/48x48/mpz.png DESTINATION share/icons/hicolor/48x48/apps/)
install(FILES app/resources/icons/32x32/mpz.png DESTINATION share/icons/hicolor/32x32/apps/)
install(FILES app/resources/icons/24x24/mpz.png DESTINATION share/icons/hicolor/24x24/apps/)
install(FILES app/resources/icons/22x22/mpz.png DESTINATION share/icons/hicolor/22x22/apps/)
install(FILES app/resources/icons/16x16/mpz.png DESTINATION share/icons/hicolor/16x16/apps/)
install(FILES license.txt DESTINATION share/licenses/${CMAKE_PROJECT_NAME})


if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(${CMAKE_PROJECT_NAME})
endif()
