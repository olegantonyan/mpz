# Google uses depot_tools to download and build crashpad. We will do the same.
# This file make assumptions about where crashpad is located. Chromium uses depot_tools
# to download and setup a directory for the crashpad/breakpad/chromium tools. We do not
# follow suit exactly, in that we assume crashpad is already available locally. We
# placed it in libs/crashpad. Inside that directory is exactly what would be expected if
# "fetch crashpad" was run in the "libs/crashpad" directory, but we remove the .git folers
# inside the downloaded "libs/crashpad/{buildtools,crashpad}" as we want it in the current,
# source tree, and not as a submodule/subtree.
include(ExternalProject)
include(FetchContent)

# Here we only download depot_tools by using FetchContent. If the developer has
# installed depot_tools on their own system and added it to path, this would
# not be necessary, but for the sake of Etterna, it was added to keep build instruction
# modification to a minimum. Unlike ExternalProject, FetchContent will download the
# git repository *during project generation*.
FetchContent_Declare(depot_tools
	GIT_REPOSITORY https://chromium.googlesource.com/chromium/tools/depot_tools.git
	GIT_PROGRESS TRUE
	GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(depot_tools)
FetchContent_GetProperties(depot_tools SOURCE_DIR DEPOT_TOOLS_DIR)

list(APPEND CMAKE_PROGRAM_PATH ${DEPOT_TOOLS_DIR})
find_program(EXE_DEPOT_GN gn)
find_program(EXE_DEPOT_NINJA ninja)

# Crashpad source and build folders
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/crashpad)
set(BINARY_DIR ${PROJECT_BINARY_DIR}/gn_crashpad)

# Assume 64bit, change to 32 if necessary
set(GN_TARGET_CPU x64)
if(CMAKE_SIZEOF_VOID_P EQUAL 4) # If 32bit
	set(GN_TARGET_CPU x86)
endif()

# Lists of the expected output libraries
if(WIN32)
	list(APPEND OUTPUT_LIBS
		${BINARY_DIR}/obj/client/client.lib
		${BINARY_DIR}/obj/util/util.lib
		${BINARY_DIR}/obj/third_party/mini_chromium/mini_chromium/base/base.lib)
elseif(UNIX)
	list(APPEND OUTPUT_LIBS
		${BINARY_DIR}/obj/client/libclient.a
		${BINARY_DIR}/obj/util/libutil.a
		${BINARY_DIR}/obj/third_party/mini_chromium/mini_chromium/base/libbase.a)
endif()

# macOS Specific Libraries - Mac requires some extra libraries to build properly
if(APPLE)
	list(APPEND OUTPUT_LIBS
		${BINARY_DIR}/obj/gen/util/mach/mig_output.child_portServer.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.child_portUser.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.excServer.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.excUser.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.mach_excServer.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.mach_excUser.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.notifyServer.o
		${BINARY_DIR}/obj/gen/util/mach/mig_output.notifyUser.o)
endif()

# gn build system options
set(GN_IS_DEBUG false)	# Build crashpad in debug or release (default is release)
set(GN_WIN_LINK_FLAG "")	# Dynamically link to C runtime library
if(WIN32)
	set(GN_WIN_LINK_FLAG /MD)	# Dynamically link to C runtime library
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug) #TODO: Fix for multi-config generators
	set(GN_IS_DEBUG true)
#	string(APPEND GN_WIN_LINK_FLAG d)
endif()

# TODO: Can this be removed? Only the steps below are used.
ExternalProject_Add(crashpad_init
	PREFIX  ${BINARY_DIR}
	SOURCE_DIR ${SOURCE_DIR}
	BINARY_DIR ${BINARY_DIR}

	# Disable all commands, since we want to run them manually.
	DOWNLOAD_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND "")

# Generate project files and build
# "gn gen build_directory && ninja -C build_directory crashpad_handler"
# This is set to always generate. If any config operations are changed, it will need to be rerun.
# Re-running gn every single time is not the most efficient, though it has a very short runtime
# on subsequent runs, and if nothing is changed the build step will do nothing.c
ExternalProject_Add_Step(crashpad_init gn_configure
	ALWAYS 1
	USES_TERMINAL 1
	WORKING_DIRECTORY ${SOURCE_DIR}
	COMMAND ${EXE_DEPOT_GN} gen ${BINARY_DIR} "--args=target_cpu=\"${GN_TARGET_CPU}\" is_debug=${GN_IS_DEBUG} extra_cflags=\"${GN_WIN_LINK_FLAG}\"" && ${EXE_DEPOT_NINJA} -C ${BINARY_DIR} crashpad_handler
	BYPRODUCTS ${OUTPUT_LIBS})

# Create library which will be linked to main application
add_library(crashpad INTERFACE)
add_dependencies(crashpad crashpad_init)
target_link_libraries(crashpad INTERFACE ${OUTPUT_LIBS})
target_include_directories(crashpad INTERFACE ${SOURCE_DIR})
target_include_directories(crashpad INTERFACE ${SOURCE_DIR}/third_party/mini_chromium/mini_chromium)

if(UNIX)
	find_package(Threads REQUIRED)
	target_link_libraries(crashpad INTERFACE Threads::Threads)
endif()

if(APPLE)
	# macOS Frameworks for crashpad
	find_library(MAC_FRAME_COCOA Cocoa)
	find_library(MAC_FRAME_IOKIT IOKit)
	find_library(MAC_FRAME_SECURITY Security)
	target_link_libraries(crashpad INTERFACE bsm)

	target_link_libraries(crashpad INTERFACE ${MAC_FRAME_COCOA})
	target_link_libraries(crashpad INTERFACE ${MAC_FRAME_IOKIT})
	target_link_libraries(crashpad INTERFACE ${MAC_FRAME_SECURITY})
endif()

# Crash Handler Executable Location
set(CRASH_HANDLER_EXE ${BINARY_DIR}/crashpad_handler)
if(WIN32)
	string(APPEND CRASH_HANDLER_EXE .exe)
endif()
