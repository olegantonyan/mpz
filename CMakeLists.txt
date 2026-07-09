cmake_minimum_required(VERSION 3.16)

project(mpz VERSION 2.0.15 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_COMPONENTS Core Gui Widgets Multimedia Concurrent Network Svg)

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

option(BUILD_TESTING "Build unit tests" OFF)

option(USE_QT5 "Use Qt5, the default is Qt6" OFF)
if (USE_QT5)
  find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
  set(QT_VERSION_MAJOR ${Qt5Core_VERSION_MAJOR})
else()
  find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})
  set(QT_VERSION_MAJOR ${Qt6Core_VERSION_MAJOR})
endif()

set(PACKAGE_VERSION "" CACHE STRING "An additional version info passed from packaging step")

set(PROJECT_SOURCES
  app/icons.cpp
  app/resources/icons/ui/bootstrap.qrc
  app/about_ui/aboutdialog.cpp
  app/busyspinner.cpp
  app/loadingspinner.cpp
  app/config/global.cpp
  app/config/local.cpp
  app/config/storage.cpp
  app/config/value.cpp
  app/coverart/embedded.cpp
  app/lyrics/cache.cpp
  app/lyrics/lrclibclient.cpp
  app/lyrics/lrcparser.cpp
  app/lyrics/lyricsovhclient.cpp
  app/lyrics/neteaseclient.cpp
  app/lyrics/provider.cpp
  app/lyrics/providerchain.cpp
  app/lyrics/qqmusicclient.cpp
  app/lyrics/textmatch.cpp
  app/directory_ui/directorycontroller.cpp
  app/directory_ui/directorysettings.cpp
  app/feedback_ui/feedbackform.cpp
  app/ipc/instance.cpp
  app/main.cpp
  app/mainmenu.cpp
  app/mpzapplication.cpp
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
  app/playlist_ui/tageditordialog.cpp
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
  app/settings_ui/settingsdialog.cpp
  app/sort_ui/sortingpresetsdialog.cpp
  app/sort_ui/sortmenu.cpp
  app/statusbarlabel.cpp
  app/sysinfo.cpp
  app/update_check/updatechecker.cpp
  app/track.cpp
  app/reveal_in_filemanager.cpp
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
  app/playlist_ui/tageditordialog.ui
  app/shortcuts_ui/shortcutsdialog.ui
  app/sort_ui/sortingpresets.ui
  app/resources.qrc
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
  if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.xml dbus/mpris.h Mpris)
    qt_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.Player.xml dbus/mpris.h Mpris)
  else()
    qt5_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.xml dbus/mpris.h Mpris)
    qt5_add_dbus_adaptor(PROJECT_SOURCES app/dbus/org.mpris.MediaPlayer2.Player.xml dbus/mpris.h Mpris)
  endif()
endif()

if (WIN32)
  if(MINGW AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
    # Force MinGW's windres; CMake 3.28+ may otherwise pick up llvm-rc / rc.exe,
    # whose flags differ from the ones below. CMAKE_RC_COMPILER_INIT must be
    # set BEFORE enable_language("RC") to influence the compiler search.
    # MSVC (used for the ARM64 build) uses rc.exe with CMake's default handling.
    set(CMAKE_RC_COMPILER_INIT windres)
    set(CMAKE_RC_FLAGS "--input-format=rc --output-format=coff")
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> --input-format=rc --output-format=coff -i <SOURCE> -o <OBJECT>")
  endif()
  enable_language("RC")
  string(TIMESTAMP MPZ_BUILD_YEAR "%Y")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons/mpz.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/mpz.rc
    @ONLY)
  list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/mpz.rc)
  list(APPEND PROJECT_SOURCES app/windows/windowstaskbar.cpp)
endif()

if (APPLE)
  list(APPEND PROJECT_SOURCES app/macos/macmediacontrols.mm app/macos/macdockmenu.cpp app/macos/macmenubar.cpp)
  set_source_files_properties(app/macos/macmediacontrols.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()

# NOT MINGW: SMTC is C++/WinRT, which the MinGW Qt5 build can't compile.
if (WIN32 AND NOT MINGW)
  list(APPEND PROJECT_SOURCES app/windows/windowsmediacontrols.cpp)
  # C++/WinRT under C++17 pulls <experimental/coroutine>, which the newest MSVC STL
  # rejects (STL1011). Build this TU as C++20; the _SILENCE define is a fallback if
  # that flag is ever overridden back to C++17.
  set_source_files_properties(app/windows/windowsmediacontrols.cpp PROPERTIES
    COMPILE_OPTIONS "/std:c++20"
    COMPILE_DEFINITIONS "_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS")
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
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

if (WIN32)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE shell32 ole32)
endif()

if (WIN32 AND NOT MINGW)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE SMTC_ENABLE)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE runtimeobject) # C++/WinRT activation factory
endif()

if (APPLE)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    "-framework IOKit" "-framework MediaPlayer" "-framework AppKit" "-framework Foundation")
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE VERSION="${PROJECT_VERSION}")
if (ENABLE_DBUS)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE MPRIS_ENABLE)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE MPZ_ENABLE_DBUS)
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

if(APPLE)
  set(_icns_file ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons/mpz.icns)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${_icns_file})
  set_source_files_properties(${_icns_file} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_ICON_FILE mpz.icns
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/app/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "mpz"
    MACOSX_BUNDLE_EXECUTABLE_NAME "mpz"
  )
endif()

if(PACKAGE_VERSION)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PACKAGE_VERSION="${PACKAGE_VERSION}")
endif()












# 3RDPARTY
include(disable_installation.cmake)
set(_DISABLE_INSTALLATION TRUE)

# YAML-CPP
option(USE_SYSTEM_YAMLCPP "Use Yaml-cpp provided by the OS" OFF)
if(USE_SYSTEM_YAMLCPP)
  find_package(yaml-cpp REQUIRED)
  message(STATUS "using system-wide yaml-cpp ${yaml-cpp_VERSION}")
  if(TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE yaml-cpp::yaml-cpp)
  else()
    # yaml-cpp < 0.8 only exports the un-namespaced target
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE yaml-cpp)
  endif()
else()
  set(YAMLCPP_DIR 3rdparty/yaml-cpp-0.9.0)
  message(STATUS "using vendored yaml-cpp from ${YAMLCPP_DIR}")
  add_subdirectory(${YAMLCPP_DIR})
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE yaml-cpp::yaml-cpp)
endif()
# ENDOF YAML-CPP

# TAGLIB
option(USE_SYSTEM_TAGLIB "Use Taglib provided by the OS" OFF)
if (USE_SYSTEM_TAGLIB)
  find_package(TagLib 1.12 REQUIRED)
  message(STATUS "using system-wide taglib ${TagLib_VERSION}")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE TagLib::tag)
else()
  set(TAGLIB_DIR 3rdparty/taglib-2.3)
  message(STATUS "using vendored taglib from ${TAGLIB_DIR}")
  add_subdirectory(${TAGLIB_DIR})

  # Upstream taglib only sets INSTALL_INTERFACE include dirs, so nothing
  # propagates to add_subdirectory consumers. Attach BUILD_INTERFACE dirs to
  # the `tag` target so consumers see the same flat header layout that the
  # system package exposes via /usr/include/taglib.
  set(_taglib_root ${CMAKE_CURRENT_SOURCE_DIR}/${TAGLIB_DIR})
  target_include_directories(tag INTERFACE
    $<BUILD_INTERFACE:${_taglib_root}/taglib>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/ape>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/asf>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/dsdiff>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/dsf>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/flac>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/it>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/mod>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/mp4>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/mpeg>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/mpeg/id3v1>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/mpeg/id3v2>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/mpeg/id3v2/frames>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/ogg>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/ogg/flac>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/ogg/opus>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/ogg/speex>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/ogg/vorbis>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/riff>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/riff/aiff>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/riff/wav>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/s3m>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/toolkit>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/trueaudio>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/wavpack>
    $<BUILD_INTERFACE:${_taglib_root}/taglib/xm>
  )
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE tag)
endif()
# ENDOF TAGLIB

# QHOTKEY
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qhotkey)
set(QHOTKEY_DIR 3rdparty/QHotkey-1.5.0)
add_subdirectory(${QHOTKEY_DIR})
message(STATUS "using vendored qhotkey from ${QHOTKEY_DIR}")
# ENDOF QHOTKEY

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

# CPPTRACE
# Linux-only crash handler. cpptrace + libdl + dladdr + -rdynamic gives
# demangled function names from STRIPPED release binaries (no DWARF shipped).
# Skipped on Windows: PE has no .dynsym equivalent for executables, so the
# same trick can't work there without shipping DWARF or PDBs.
if(UNIX AND NOT APPLE)
  set(CPPTRACE_DIR 3rdparty/cpptrace-1.0.4)
  set(CPPTRACE_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(CPPTRACE_GET_SYMBOLS_WITH_LIBDL ON CACHE BOOL "" FORCE)
  set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "" FORCE)
  set(CPPTRACE_UNWIND_WITH_UNWIND ON CACHE BOOL "" FORCE)
  set(CPPTRACE_DEMANGLE_WITH_CXXABI ON CACHE BOOL "" FORCE)
  add_subdirectory(${CPPTRACE_DIR})
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE app/crash_handler.cpp)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE cpptrace::cpptrace)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_CRASH_HANDLER)
  target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -rdynamic)
endif()
# ENDOF CPPTRACE

set(_DISABLE_INSTALLATION FALSE)
# ENDOF 3RDPARTY













include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(NOT APPLE AND NOT WIN32)
  install(FILES mpz.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES license.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${CMAKE_PROJECT_NAME})
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons/mpz.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  file(GLOB ICON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/app/resources/icons/*x*/mpz.png")
  foreach(icon IN LISTS ICON_FILES)
    get_filename_component(size_dir "${icon}" DIRECTORY)
    get_filename_component(size_dir "${size_dir}" NAME)
    string(REGEX MATCH "^[0-9]+" icon_size "${size_dir}")
    if(icon_size LESS_EQUAL 48)
      install(FILES "${icon}" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size_dir}/apps)
    endif()
  endforeach()
endif()


if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(${CMAKE_PROJECT_NAME})
endif()

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()
