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
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${name} PRIVATE -ffunction-sections -fdata-sections)
    if(APPLE)
      target_link_options(${name} PRIVATE -Wl,-dead_strip)
    else()
      target_link_options(${name} PRIVATE -Wl,--gc-sections)
    endif()
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
mpz_add_test(tst_lyricstextmatch tst_lyricstextmatch.cpp ${APP}/lyrics/textmatch.cpp)
mpz_add_test(tst_streammetadata tst_streammetadata.cpp ${APP}/streammetadata.cpp)
mpz_add_test(tst_configvalue    tst_configvalue.cpp    ${APP}/config/value.cpp)
mpz_add_test(tst_treeitem       tst_treeitem.cpp       ${APP}/directory_ui/directorymodel/mpd/treeitem.cpp)
mpz_add_test(tst_radioitem      tst_radioitem.cpp      ${APP}/directory_ui/directorymodel/radio/item.cpp)
mpz_add_test(tst_crashreport    tst_crashreport.cpp    ${APP}/crash_report/crashreport.cpp)
mpz_add_test(tst_pcmcache       tst_pcmcache.cpp       ${APP}/playback/gapless/pcmcache.cpp)
mpz_add_test(tst_eqbiquad       tst_eqbiquad.cpp)
mpz_add_test(tst_equalizer      tst_equalizer.cpp      ${APP}/eq/equalizer.cpp)
mpz_add_test(tst_autoeqimport   tst_autoeqimport.cpp   ${APP}/eq/autoeqimport.cpp)
mpz_add_test(tst_eqexport       tst_eqexport.cpp       ${APP}/eq/eqexport.cpp ${APP}/eq/autoeqimport.cpp ${APP}/eq/equalizer.cpp)

# SysInfo needs <QApplication> headers for qApp; pulls in Qt::Widgets for the
# include path only (the test uses QCoreApplication).
mpz_add_test(tst_sysinfo tst_sysinfo.cpp ${APP}/sysinfo.cpp)
target_link_libraries(tst_sysinfo PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# ProviderChain statics (knownProviders/displayName) are pure; the .cpp includes
# the network client headers, but --gc-sections drops the unreferenced
# makeProvider/fetch bodies so only QtNetwork include paths are needed.
mpz_add_test(tst_providerchain tst_providerchain.cpp ${APP}/lyrics/providerchain.cpp)
target_link_libraries(tst_providerchain PRIVATE Qt${QT_VERSION_MAJOR}::Network)

# Same shape as tst_providerchain, for the cover chain.
mpz_add_test(tst_coverproviderchain tst_coverproviderchain.cpp ${APP}/coverart/online/providerchain.cpp)
target_link_libraries(tst_coverproviderchain PRIVATE Qt${QT_VERSION_MAJOR}::Network)

# Album scoring reuses the lyrics text normalizers; both are pure.
mpz_add_test(tst_coveralbummatch tst_coveralbummatch.cpp
  ${APP}/coverart/online/albummatch.cpp
  ${APP}/lyrics/textmatch.cpp)

# Local folder-cover ranking is pure; QString/QFileInfo/QRegularExpression only.
mpz_add_test(tst_foldercover tst_foldercover.cpp ${APP}/coverart/foldercover.cpp)

# Both online caches write real files under QStandardPaths::CacheLocation via
# the shared DiskCache::Store. The cover test builds a sample image, so it also
# needs Qt::Gui.
mpz_add_test(tst_coveronlinecache tst_coveronlinecache.cpp
  ${APP}/coverart/online/cache.cpp
  ${APP}/diskcache.cpp
  ${APP}/lyrics/textmatch.cpp)
target_link_libraries(tst_coveronlinecache PRIVATE Qt${QT_VERSION_MAJOR}::Gui)

mpz_add_test(tst_lyricscache tst_lyricscache.cpp
  ${APP}/lyrics/cache.cpp
  ${APP}/diskcache.cpp
  ${APP}/lyrics/textmatch.cpp)

# Radio catalog parsing is pure -- Catalog takes its user-file path from a
# setter rather than Config::Storage, so these need neither yaml-cpp nor TagLib.
mpz_add_test(tst_radiocatalog tst_radiocatalog.cpp
  ${APP}/radio/catalog.cpp
  ${APP}/radio/station.cpp)

# Guards the shipped station list. Reads the source json directly, so no qrc.
mpz_add_test(tst_radiocatalogresource tst_radiocatalogresource.cpp
  ${APP}/radio/catalog.cpp
  ${APP}/radio/station.cpp)
target_compile_definitions(tst_radiocatalogresource PRIVATE
  RADIO_STATIONS_JSON="${CMAKE_SOURCE_DIR}/app/resources/radio/radio.json")

# resolver.cpp includes QtNetwork for the remote fetch; firstStreamUrl/
# looksLikePlaylist are pure and the unreferenced fetch body is dropped by
# --gc-sections, so only the include paths are needed.
mpz_add_test(tst_radioresolver tst_radioresolver.cpp ${APP}/radio/resolver.cpp)
target_link_libraries(tst_radioresolver PRIVATE Qt${QT_VERSION_MAJOR}::Network)

# Same deal for icecaststatus.cpp: nowPlayingFromStatusJson/statusJsonUrl are pure and
# the IcecastStatus poller body is unreferenced by the test.
mpz_add_test(tst_icecaststatus tst_icecaststatus.cpp ${APP}/radio/icecaststatus.cpp)
target_link_libraries(tst_icecaststatus PRIVATE Qt${QT_VERSION_MAJOR}::Network)

# UpdateChecker::isNewer is a static semver comparison. updatechecker.cpp pulls
# <QApplication> and a QNetworkAccessManager member for its include paths; the
# networking/qApp code is unreferenced by the test and dropped by --gc-sections.
mpz_add_test(tst_updatechecker tst_updatechecker.cpp ${APP}/update_check/updatechecker.cpp)
target_link_libraries(tst_updatechecker PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Network)

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

# Timeline stores Track by value, so it compiles track.cpp (TagLib/yaml-cpp
# include paths only; --gc-sections drops the unreferenced bodies).
mpz_add_test(tst_gaplesstimeline tst_gaplesstimeline.cpp
  ${APP}/playback/gapless/timeline.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_gaplesstimeline PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

# PendingNext stores a Track by value, so it compiles track.cpp (same
# TagLib/yaml-cpp include-path-only shape as tst_gaplesstimeline).
mpz_add_test(tst_pendingnext tst_pendingnext.cpp
  ${APP}/playback/pendingnext.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_pendingnext PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

mpz_add_test(tst_sorter tst_sorter.cpp
  ${APP}/playlist/sorter.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_sorter PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)

mpz_add_test(tst_playlist tst_playlist.cpp
  ${APP}/playlist/playlist.cpp
  ${APP}/playlist/sorter.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_playlist PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Concurrent tag yaml-cpp::yaml-cpp)

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

# Config::Global wraps Storage; transitively pulls Track/columnsconfig via
# headers, but --gc-sections drops unreferenced bodies (see tst_track note).
mpz_add_test(tst_globalconfig tst_globalconfig.cpp
  ${APP}/config/global.cpp
  ${APP}/config/storage.cpp
  ${APP}/config/value.cpp
  ${APP}/playlist_ui/columnsconfig.cpp
  ${APP}/radio/station.cpp
  ${APP}/track.cpp
  ${APP}/rnjesus.cpp
  ${APP}/streammetadata.cpp)
target_link_libraries(tst_globalconfig PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui tag yaml-cpp::yaml-cpp)
target_compile_definitions(tst_globalconfig PRIVATE VERSION="${PROJECT_VERSION}")

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
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Concurrent tag yaml-cpp::yaml-cpp)

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
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Concurrent tag yaml-cpp::yaml-cpp)
