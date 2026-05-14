#ifndef MPZ_CRASH_HANDLER_H
#define MPZ_CRASH_HANDLER_H

namespace mpz {

// Installs sigaction-based handlers for SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL.
// On fault, prints a cpptrace stack trace to stderr and exits with 128+signum.
// Symbol resolution uses libdl + dladdr; combined with the -rdynamic link flag
// this produces demangled function names from stripped release binaries with
// no DWARF shipped.
void install_crash_handler();

}

#endif
