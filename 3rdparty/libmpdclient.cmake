set(LIBMPDCLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libmpdclient-2.23)
message(STATUS "using vendored libmpdclient from ${LIBMPDCLIENT_DIR}")

set(MPDCLIENT_VERSION "2.23")
# Generate config.h
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFile)
check_include_file("sys/socket.h"     HAVE_SYS_SOCKET_H)
check_include_file("netinet/in.h"     HAVE_NETINET_IN_H)
check_include_file("arpa/inet.h"      HAVE_ARPA_INET_H)
check_include_file("unistd.h"         HAVE_UNISTD_H)
check_include_file("stdlib.h"         HAVE_STDLIB_H)
check_include_file("string.h"         HAVE_STRING_H)
check_include_file("locale.h"         HAVE_LOCALE_H)
check_include_file("netdb.h"          HAVE_NETDB_H)
check_symbol_exists(strndup "string.h" HAVE_STRNDUP)
check_function_exists(setlocale HAVE_SETLOCALE)
check_function_exists(uselocale HAVE_USELOCALE)
check_symbol_exists(getaddrinfo "netdb.h" HAVE_GETADDRINFO)

set(CONFIG_H_FILE ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/config.h)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated)

file(WRITE ${CONFIG_H_FILE} "
#pragma once
#define DEFAULT_HOST \"localhost\"
#define DEFAULT_PORT 6600
#define DEFAULT_SOCKET \"/run/mpd/socket\"
#define PACKAGE \"libmpdclient\"
")
if(HAVE_GETADDRINFO)
  file(APPEND ${CONFIG_H_FILE} "#define HAVE_GETADDRINFO\n")
endif()
if(HAVE_SETLOCALE)
  file(APPEND ${CONFIG_H_FILE} "#define HAVE_SETLOCALE\n#include <locale.h>\n")
endif()
if(HAVE_STRNDUP)
  file(APPEND ${CONFIG_H_FILE} "#define HAVE_STRNDUP\n#include <string.h>\n")
endif()
if(HAVE_USELOCALE)
  file(APPEND ${CONFIG_H_FILE} "#define HAVE_USELOCALE\n#include <locale.h>\n")
endif()
if(NOT MSVC)
  file(APPEND ${CONFIG_H_FILE} "#ifndef _GNU_SOURCE\n#define _GNU_SOURCE\n#endif")
endif()

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
