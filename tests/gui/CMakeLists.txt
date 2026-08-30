# GUI tests drive the real widgets. Separate target and ctest label: they need a
# display, a session bus and an audio sink, so the plain unit run neither builds
# nor executes them.
add_custom_target(build_gui_tests)

function(mpz_add_gui_test name)
  mpz_test_target(${name})
  target_compile_definitions(${name} PRIVATE
    AUDIO_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/replaygain")
  add_dependencies(build_gui_tests ${name})
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES
    LABELS gui
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

mpz_add_gui_test(tst_mainwindow)
mpz_add_gui_test(tst_playlists)
mpz_add_gui_test(tst_playlisttracks)
mpz_add_gui_test(tst_library)
