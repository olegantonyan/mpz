set(LIBMPDCLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libmpdclient-2.26)
set(MAJOR_VERSION 2)
set(MINOR_VERSION 26)
set(PATCH_VERSION 0)

message(STATUS "using vendored libmpdclient from ${LIBMPDCLIENT_DIR}")

# Generate config.h
include(CheckSymbolExists)
include(CheckFunctionExists)
if(WIN32)
  check_symbol_exists(getaddrinfo "ws2tcpip.h" HAVE_GETADDRINFO)
else()
  check_symbol_exists(getaddrinfo "netdb.h" HAVE_GETADDRINFO)
endif()
check_symbol_exists(strndup "string.h" HAVE_STRNDUP)
check_function_exists(uselocale HAVE_USELOCALE)

set(CONFIG_H_FILE ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/config.h)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated)

set(CONFIG_H_BODY "#ifndef __LIBMPD_CONFIG_H_FILE\n#define __LIBMPD_CONFIG_H_FILE\n")
# _GNU_SOURCE is a feature-test macro: it must precede any libc header to have any effect
if(NOT MSVC)
  string(APPEND CONFIG_H_BODY "\n#ifndef _GNU_SOURCE\n#define _GNU_SOURCE\n#endif\n")
endif()
string(APPEND CONFIG_H_BODY "
#define DEFAULT_HOST \"localhost\"
#define DEFAULT_PORT 6600
#define DEFAULT_SOCKET \"/run/mpd/socket\"
#define PACKAGE \"libmpdclient\"
")
if(HAVE_GETADDRINFO)
  string(APPEND CONFIG_H_BODY "#define HAVE_GETADDRINFO\n")
endif()
if(HAVE_STRNDUP)
  string(APPEND CONFIG_H_BODY "#define HAVE_STRNDUP\n")
endif()
if(HAVE_USELOCALE)
  string(APPEND CONFIG_H_BODY "#define HAVE_USELOCALE\n")
endif()
string(APPEND CONFIG_H_BODY "\n#endif\n")
file(WRITE ${CONFIG_H_FILE} "${CONFIG_H_BODY}")

# version.h
configure_file(${LIBMPDCLIENT_DIR}/include/mpd/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/mpd/version.h @ONLY)

# sources
file(GLOB_RECURSE MPDCLIENT_SOURCES ${LIBMPDCLIENT_DIR}/src/*.c)
list(REMOVE_ITEM MPDCLIENT_SOURCES "${LIBMPDCLIENT_DIR}/src/example.c")
file(GLOB_RECURSE MPDCLIENT_HEADERS ${LIBMPDCLIENT_DIR}/include/*.h)

add_library(mpdclient STATIC
  ${MPDCLIENT_SOURCES}
  ${MPDCLIENT_HEADERS}
  ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/mpd/version.h
  ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated/config.h
)
target_compile_definitions(mpdclient PRIVATE ENABLE_TCP=1)
# target_compile_definitions(mpdclient PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200112L)

target_include_directories(mpdclient PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/mpd/generated
  ${LIBMPDCLIENT_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(mpdclient PUBLIC Threads::Threads)
if(WIN32)
  target_link_libraries(mpdclient PRIVATE ws2_32)
endif()
