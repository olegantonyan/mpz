#ifndef MPZ_CRASH_HANDLER_H
#define MPZ_CRASH_HANDLER_H

#include <string>

namespace mpz {

// On POSIX, installs std::signal handlers for SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL;
// on Windows, a SetUnhandledExceptionFilter for SEH faults plus a SIGABRT handler.
// Both also set a std::terminate handler. On fault, prints a cpptrace stack trace
// to stderr (and appends it to the crash-log file when one is set). Symbol
// resolution: Linux -rdynamic .dynsym, macOS Mach-O symbol table, Windows the
// mpz.pdb shipped beside mpz.exe (read via dbghelp) — demangled names either way.
void install_crash_handler();

// Registers the file that each crash trace is appended to, separated by a banner.
// Call once after the application name is set. The parent directory is created
// lazily on the first crash, not here.
void set_crash_log_path(std::string path);

// Returns the crash-log file path set via set_crash_log_path(), or empty if unset.
std::string crash_log_path();

// Registers a pre-rendered system-info block written into each crash item.
// Call once at startup after the application version is set. Safe to omit.
void set_system_info(std::string info);

}

#endif
