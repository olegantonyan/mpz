set(SQLITE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sqlite-amalgamation-3530400)

# only the library; shell.c ships in the upstream archive but is the CLI
add_library(sqlite3 STATIC EXCLUDE_FROM_ALL ${SQLITE_DIR}/sqlite3.c)

target_include_directories(sqlite3 PUBLIC ${SQLITE_DIR})

# mpz sets AUTOMOC globally, and moc has nothing to do in a C target
set_target_properties(sqlite3 PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

target_compile_definitions(sqlite3 PRIVATE
  SQLITE_THREADSAFE=1
  SQLITE_DQS=0
  SQLITE_DEFAULT_MEMSTATUS=0
  SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
  SQLITE_LIKE_DOESNT_MATCH_BLOBS
  SQLITE_MAX_EXPR_DEPTH=0
  SQLITE_OMIT_DEPRECATED
  SQLITE_OMIT_LOAD_EXTENSION
  SQLITE_OMIT_SHARED_CACHE
  SQLITE_OMIT_PROGRESS_CALLBACK
)

find_package(Threads REQUIRED)
target_link_libraries(sqlite3 PUBLIC Threads::Threads)
if(NOT WIN32)
  target_link_libraries(sqlite3 PUBLIC ${CMAKE_DL_LIBS} m)
endif()

# false positive in sqlite3ColumnSetColl: gcc thinks the inlined strlen reads from address zero
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(sqlite3 PRIVATE -Wno-stringop-overread)
endif()
