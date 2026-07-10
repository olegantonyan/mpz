#ifndef MPZ_CRASH_HANDLER_H
#define MPZ_CRASH_HANDLER_H

#include <string>

namespace mpz {

// Installs std::signal handlers for SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL plus a
// std::terminate handler. On fault, prints a cpptrace stack trace to stderr (and
// appends it to the crash-log file when one is set) and exits with 128+signum.
// Symbol resolution uses libdl + dladdr: on Linux the -rdynamic link flag exports
// symbols into .dynsym; on macOS dladdr reads the retained Mach-O symbol table.
// Either way it yields demangled function names from stripped release binaries
// with no DWARF shipped.
void install_crash_handler();

// Registers the file that each crash trace is appended to, separated by a banner.
// Call once after the application name is set. The parent directory is created
// lazily on the first crash, not here.
void set_crash_log_path(std::string path);

// Returns the crash-log file path set via set_crash_log_path(), or empty if unset.
std::string crash_log_path();

}

#endif
