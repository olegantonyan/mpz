find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

add_custom_target(build_tests)

# Every test links the whole app object library, so it inherits the app/ include
# dir, the feature defines and every 3rdparty dependency from mpz_lib. Paired
# with mpz_lib's -ffunction-sections, this drops what a given test never calls
# instead of giving each binary a full copy of the app.
function(mpz_test_target name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE ${MPZ_LIB} Qt${QT_VERSION_MAJOR}::Test)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(APPLE)
      target_link_options(${name} PRIVATE -Wl,-dead_strip)
    else()
      target_link_options(${name} PRIVATE -Wl,--gc-sections)
    endif()
  endif()
endfunction()

function(mpz_add_test name)
  mpz_test_target(${name})
  add_dependencies(build_tests ${name})
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

mpz_add_test(tst_playerstate)
mpz_add_test(tst_randomtrail)
mpz_add_test(tst_rnjesus)
mpz_add_test(tst_lrcparser)
mpz_add_test(tst_lyricstextmatch)
mpz_add_test(tst_streammetadata)
mpz_add_test(tst_configvalue)
mpz_add_test(tst_radioitem)
mpz_add_test(tst_crashreport)
mpz_add_test(tst_waveformpeaks)
mpz_add_test(tst_eqbiquad)
mpz_add_test(tst_replaygaingain)
mpz_add_test(tst_sysinfo)
mpz_add_test(tst_providerchain)
mpz_add_test(tst_coverproviderchain)
mpz_add_test(tst_coveralbummatch)
mpz_add_test(tst_foldercover)
mpz_add_test(tst_coveronlinecache)
mpz_add_test(tst_lyricscache)
mpz_add_test(tst_radiocatalog)
mpz_add_test(tst_radioresolver)
mpz_add_test(tst_icecaststatus)
mpz_add_test(tst_track)
mpz_add_test(tst_pendingnext)
mpz_add_test(tst_sorter)
mpz_add_test(tst_playlist)
mpz_add_test(tst_columnsconfig)
mpz_add_test(tst_configstorage)
mpz_add_test(tst_globalconfig)
mpz_add_test(tst_localconfig)
mpz_add_test(tst_shortcuts)
mpz_add_test(tst_fileparser)
mpz_add_test(tst_cueparser)
mpz_add_test(tst_loader)
mpz_add_test(tst_backgroundtasks)
mpz_add_test(tst_dropdirs)
mpz_add_test(tst_tracksmimedata)
mpz_add_test(tst_radiolibrary)
mpz_add_test(tst_sortpresets)
mpz_add_test(tst_ipcinstance)
mpz_add_test(tst_playbacklogmodel)
mpz_add_test(tst_modusoperandi)
mpz_add_test(tst_playlistuimodel)
mpz_add_test(tst_playlistsuimodel)
mpz_add_test(tst_playlistproxyfilter)
mpz_add_test(tst_playlistsproxyfilter)
mpz_add_test(tst_directoryproxy)
mpz_add_test(tst_dispatch)
mpz_add_test(tst_statusbarlabel)

mpz_add_test(tst_cueparser_fixtures)
target_compile_definitions(tst_cueparser_fixtures PRIVATE
  CUE_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cue")

# Guards the shipped station list. Reads the source json directly, so no qrc.
mpz_add_test(tst_radiocatalogresource)
target_compile_definitions(tst_radiocatalogresource PRIVATE
  RADIO_STATIONS_JSON="${CMAKE_SOURCE_DIR}/app/resources/radio/radio.json")

if(ENABLE_UPDATE_CHECK)
  mpz_add_test(tst_updatechecker)
endif()

# MPD data classes — testable for defaults + nullptr update guard.
# Constructing real mpd_status/mpd_song/mpd_output structs requires
# libmpdclient internals, so deeper coverage would need a live mpd server.
if(ENABLE_MPD_SUPPORT)
  mpz_add_test(tst_treeitem)
  mpz_add_test(tst_mpdentity)
  mpz_add_test(tst_mpdoutput)
  mpz_add_test(tst_mpdstatus)
  mpz_add_test(tst_mpdsong)
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  mpz_add_test(tst_dynamicrange)
  mpz_add_test(tst_drlogformat)

  mpz_add_test(tst_nativeformat)
  target_compile_definitions(tst_nativeformat PRIVATE
    AUDIO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/replaygain")
endif()

if(ENABLE_GAPLESS)
  mpz_add_test(tst_pcmcache)
  mpz_add_test(tst_gaplesstimeline)
  mpz_add_test(tst_equalizer)
  mpz_add_test(tst_autoeqimport)
  mpz_add_test(tst_eqexport)
  mpz_add_test(tst_replaygainstore)
  mpz_add_test(tst_replaygainanalyzer)
  mpz_add_test(tst_replaygainjobcodec)
  mpz_add_test(tst_replaygainscanner)
  mpz_add_test(tst_replaygainmanager)
  mpz_add_test(tst_scanworkerargs)

  mpz_add_test(tst_replaygaintags)
  target_compile_definitions(tst_replaygaintags PRIVATE
    RG_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/replaygain")
endif()

add_subdirectory(gui)
