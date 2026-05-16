find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

add_custom_target(build_tests)

# Link-time dead-code elimination keeps tests light. Each test target lists
# only the .cpp files it needs from app/; -ffunction-sections + --gc-sections
# (Linux/GNU/Clang) lets us drop unreferenced functions inside those .cpp's,
# so tests don't have to link TagLib / CoverArt / etc. just because a few
# Track methods happen to call them.
function(mpz_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Core)
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/app)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE)
    target_compile_options(${name} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${name} PRIVATE -Wl,--gc-sections)
  endif()
  add_dependencies(build_tests ${name})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

set(APP ${CMAKE_SOURCE_DIR}/app)

# Pure-logic, zero-fixture, zero-3rdparty
mpz_add_test(tst_playerstate    tst_playerstate.cpp    ${APP}/playback/playerstate.cpp)
mpz_add_test(tst_randomtrail    tst_randomtrail.cpp    ${APP}/playback/randomtrail.cpp)
mpz_add_test(tst_rnjesus        tst_rnjesus.cpp        ${APP}/rnjesus.cpp)
mpz_add_test(tst_lrcparser      tst_lrcparser.cpp      ${APP}/lyrics/lrcparser.cpp)
mpz_add_test(tst_streammetadata tst_streammetadata.cpp ${APP}/streammetadata.cpp)
mpz_add_test(tst_configvalue    tst_configvalue.cpp    ${APP}/config/value.cpp)
mpz_add_test(tst_treeitem       tst_treeitem.cpp       ${APP}/directory_ui/directorymodel/mpd/treeitem.cpp)

# SysInfo needs <QApplication> headers for qApp; pulls in Qt::Widgets for the
# include path only (the test uses QCoreApplication).
mpz_add_test(tst_sysinfo tst_sysinfo.cpp ${APP}/sysinfo.cpp)
target_link_libraries(tst_sysinfo PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Tests that compile track.cpp pull TagLib + yaml-cpp transitively through
# coverart/covers.h → modusoperandi.h → config/local.h → storage.h. With
# --gc-sections the unreferenced TagLib/CoverArt calls inside track.o get
# dropped from the final link, so we only need the include dirs (via the
# target_link_libraries below, which also keeps the libs as deps but doesn't
# inflate the test binaries).
mpz_add_test(tst_track tst_track.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_track PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

mpz_add_test(tst_sorter tst_sorter.cpp
  ${APP}/playlist/sorter.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_sorter PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

mpz_add_test(tst_columnsconfig tst_columnsconfig.cpp
  ${APP}/playlist_ui/columnsconfig.cpp
  ${APP}/config/value.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_columnsconfig PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

# Config::Storage YAML round-trips need yaml-cpp and the VERSION macro.
mpz_add_test(tst_configstorage tst_configstorage.cpp
  ${APP}/config/storage.cpp
  ${APP}/config/value.cpp)
target_link_libraries(tst_configstorage PRIVATE yaml-cpp::yaml-cpp)
target_compile_definitions(tst_configstorage PRIVATE VERSION="${PROJECT_VERSION}")

# MPD data classes — testable for defaults + nullptr update guard.
# Constructing real mpd_status/mpd_song/mpd_output structs requires
# libmpdclient internals, so deeper coverage would need a live mpd server.
if(ENABLE_MPD_SUPPORT)
  mpz_add_test(tst_mpdentity tst_mpdentity.cpp ${APP}/mpd_client/entity.cpp)
  target_link_libraries(tst_mpdentity PRIVATE mpdclient)

  mpz_add_test(tst_mpdoutput tst_mpdoutput.cpp ${APP}/mpd_client/output.cpp)
  target_link_libraries(tst_mpdoutput PRIVATE mpdclient)

  mpz_add_test(tst_mpdstatus tst_mpdstatus.cpp ${APP}/mpd_client/status.cpp)
  target_link_libraries(tst_mpdstatus PRIVATE mpdclient)

  mpz_add_test(tst_mpdsong tst_mpdsong.cpp ${APP}/mpd_client/song.cpp)
  target_link_libraries(tst_mpdsong PRIVATE mpdclient)
endif()

# Parser tests with on-disk fixtures created via QTemporaryDir.
# FileParser uses Track(filepath) which exercises TagLib, but on
# dummy-content audio files TagLib::FileRef returns null and the parser
# still produces Track objects with the correct path.
mpz_add_test(tst_fileparser tst_fileparser.cpp
  ${APP}/playlist/fileparser.cpp
  ${APP}/playlist/loader.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_fileparser PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

mpz_add_test(tst_cueparser tst_cueparser.cpp
  ${APP}/playlist/cueparser.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_cueparser PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

# Real-world cue fixtures live under tests/fixtures/cue/. The path is injected
# at build time so the test executable can find them regardless of CWD.
mpz_add_test(tst_cueparser_fixtures tst_cueparser_fixtures.cpp
  ${APP}/playlist/cueparser.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_cueparser_fixtures PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)
target_compile_definitions(tst_cueparser_fixtures PRIVATE
  CUE_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cue")

mpz_add_test(tst_loader tst_loader.cpp
  ${APP}/playlist/loader.cpp
  ${APP}/playlist/fileparser.cpp
  ${APP}/playlist/cueparser.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_loader PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)
