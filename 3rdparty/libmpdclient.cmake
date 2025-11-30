set(LIBMPDCLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libmpdclient-2.23)
message(STATUS "using vendored libmpdclient from ${LIBMPDCLIENT_DIR}")

set(MPDCLIENT_VERSION "2.23")
# Generate config.h
include(CheckIncludeFile)
include(CheckSymbolExists)
check_include_file("sys/socket.h"     HAVE_SYS_SOCKET_H)
check_include_file("netinet/in.h"     HAVE_NETINET_IN_H)
check_include_file("arpa/inet.h"      HAVE_ARPA_INET_H)
check_include_file("unistd.h"         HAVE_UNISTD_H)
check_include_file("stdlib.h"         HAVE_STDLIB_H)
check_include_file("string.h"         HAVE_STRING_H)
check_symbol_exists(strndup "string.h" HAVE_STRNDUP)
check_include_file("netdb.h"          HAVE_NETDB_H)
check_symbol_exists(getaddrinfo "netdb.h" HAVE_GETADDRINFO)
set(CONFIG_H_CONTENT
"#pragma once
#cmakedefine01 HAVE_SYS_SOCKET_H
#cmakedefine01 HAVE_NETINET_IN_H
#cmakedefine01 HAVE_ARPA_INET_H
#cmakedefine01 HAVE_UNISTD_H
#cmakedefine01 HAVE_STDLIB_H
#cmakedefine01 HAVE_STRING_H
#cmakedefine01 HAVE_STRNDUP
#cmakedefine01 HAVE_NETDB_H
#cmakedefine01 HAVE_GETADDRINFO
#define DEFAULT_HOST \"localhost\"
#define DEFAULT_PORT 6600
")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h.in "${CONFIG_H_CONTENT}")
configure_file(${CMAKE_CURRENT_BINARY_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/config.h)

# version.h
configure_file(${LIBMPDCLIENT_DIR}/include/mpd/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/mpd/version.h @ONLY)

# sources
file(GLOB_RECURSE MPDCLIENT_SOURCES ${LIBMPDCLIENT_DIR}/src/*.c)
file(GLOB_RECURSE MPDCLIENT_HEADERS ${LIBMPDCLIENT_DIR}/include/*.h)

add_library(mpdclient STATIC
  ${MPDCLIENT_SOURCES}
  ${MPDCLIENT_HEADERS}
  ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/mpd/version.h
  ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/config.h
)
target_compile_definitions(mpdclient PRIVATE _GNU_SOURCE ENABLE_TCP=1)
# target_compile_definitions(mpdclient PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200112L)

target_include_directories(mpdclient PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated
  ${LIBMPDCLIENT_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(mpdclient PUBLIC Threads::Threads)
