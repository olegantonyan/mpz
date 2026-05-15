find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

add_custom_target(build_tests)

function(mpz_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Core)
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/app)
  add_dependencies(build_tests ${name})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

mpz_add_test(tst_playerstate    tst_playerstate.cpp    ${CMAKE_SOURCE_DIR}/app/playback/playerstate.cpp)
mpz_add_test(tst_randomtrail    tst_randomtrail.cpp    ${CMAKE_SOURCE_DIR}/app/playback/randomtrail.cpp)
mpz_add_test(tst_rnjesus        tst_rnjesus.cpp        ${CMAKE_SOURCE_DIR}/app/rnjesus.cpp)
mpz_add_test(tst_lrcparser      tst_lrcparser.cpp      ${CMAKE_SOURCE_DIR}/app/lyrics/lrcparser.cpp)
mpz_add_test(tst_streammetadata tst_streammetadata.cpp ${CMAKE_SOURCE_DIR}/app/streammetadata.cpp)
