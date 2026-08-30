# MPD tests drive a real mpd server that the fixture spawns per binary. Separate
# target and ctest label: the plain unit run has no mpd to talk to, and the
# full-app cases need the same bus and audio sink the gui tests need.
add_custom_target(build_mpd_tests)

function(mpz_add_mpd_test name)
  mpz_test_target(${name})
  # tests/gui/fixture.h carries ConfigDir and MPZ_GUI_TEST_MAIN; both are needed
  # here and neither is worth a second copy.
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/tests/gui)
  target_compile_definitions(${name} PRIVATE
    AUDIO_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/replaygain")
  add_dependencies(build_mpd_tests ${name})
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    LABELS mpd
    TIMEOUT 300
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

mpz_add_mpd_test(tst_mpdconnection)
mpz_add_mpd_test(tst_mpdcommands)
mpz_add_mpd_test(tst_mpdplayback)
mpz_add_mpd_test(tst_mpdmodels)
mpz_add_mpd_test(tst_mpdplayer)
mpz_add_mpd_test(tst_mpdmainwindow)
