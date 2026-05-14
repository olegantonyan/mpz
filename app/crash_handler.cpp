#include "crash_handler.h"

#include <cpptrace/cpptrace.hpp>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace mpz {

namespace {

const char *signal_name(int signum) {
  switch (signum) {
    case SIGSEGV: return "SIGSEGV";
    case SIGABRT: return "SIGABRT";
    case SIGFPE:  return "SIGFPE";
    case SIGILL:  return "SIGILL";
#ifdef SIGBUS
    case SIGBUS:  return "SIGBUS";
#endif
    default:      return "?";
  }
}

[[noreturn]] void crash_handler(int signum) {
  // Restore default disposition so a second fault while we're tracing aborts
  // hard instead of recursing. Both glibc and Windows MSVCRT reset to SIG_DFL
  // automatically after a signal() handler fires, but be explicit anyway.
  std::signal(signum, SIG_DFL);

  // async-signal-unsafe but pragmatic — same trade-off as DeathHandler. A
  // desktop app crashing once is fine to take a few hundred ms in the handler
  // if it gets a usable trace out of it.
  std::fprintf(stderr, "\nFatal: %s (%d)\n\n", signal_name(signum), signum);
  cpptrace::generate_trace(/*skip=*/1).print(std::cerr);

  std::_Exit(128 + signum);
}

}

void install_crash_handler() {
  std::signal(SIGSEGV, &crash_handler);
  std::signal(SIGABRT, &crash_handler);
  std::signal(SIGFPE,  &crash_handler);
  std::signal(SIGILL,  &crash_handler);
#ifdef SIGBUS
  std::signal(SIGBUS,  &crash_handler);
#endif
}

}
