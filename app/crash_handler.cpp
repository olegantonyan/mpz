#include "crash_handler.h"

#include <cpptrace/cpptrace.hpp>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace mpz {

namespace {

std::string g_log_path;

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

void write_trace(const cpptrace::stacktrace &trace, const char *reason) {
  std::fprintf(stderr, "\nFatal: %s\n\n", reason);
  trace.print(std::cerr);

  if (g_log_path.empty()) return;
  std::ofstream ofs(g_log_path, std::ios::app);
  if (!ofs) return;
  ofs << "\n\n======== Fatal: " << reason << " @ " << std::time(nullptr)
      << " ========\n\n";
  trace.print(ofs);
}

[[noreturn]] void crash_handler(int signum) {
  // Restore default disposition so a second fault while we're tracing aborts
  // hard instead of recursing. Both glibc and Windows MSVCRT reset to SIG_DFL
  // automatically after a signal() handler fires, but be explicit anyway.
  std::signal(signum, SIG_DFL);

  // async-signal-unsafe but pragmatic — same trade-off as DeathHandler. A
  // desktop app crashing once is fine to take a few hundred ms in the handler
  // if it gets a usable trace out of it.
  char reason[32];
  std::snprintf(reason, sizeof(reason), "%s (%d)", signal_name(signum), signum);
  write_trace(cpptrace::generate_trace(/*skip=*/1), reason);

  std::_Exit(128 + signum);
}

[[noreturn]] void terminate_handler() {
  write_trace(cpptrace::generate_trace(/*skip=*/1), "std::terminate");
  std::abort();
}

#ifdef _WIN32
// A real access violation is delivered as an SEH exception, not a C signal, so
// the std::signal handlers below never fire for it on Windows. This top-level
// filter runs before stack unwinding, so generate_trace() still sees the faulting
// frames (rooted at the filter itself). cpptrace installs no such filter itself.
LONG WINAPI seh_filter(EXCEPTION_POINTERS *info) {
  char reason[64];
  std::snprintf(reason, sizeof(reason), "SEH exception 0x%08lX",
                info->ExceptionRecord->ExceptionCode);
  write_trace(cpptrace::generate_trace(), reason);
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

}

#ifdef _WIN32
void install_crash_handler() {
  SetUnhandledExceptionFilter(&seh_filter);
  std::set_terminate(&terminate_handler);
  std::signal(SIGABRT, &crash_handler);
}
#else
void install_crash_handler() {
  std::signal(SIGSEGV, &crash_handler);
  std::signal(SIGABRT, &crash_handler);
  std::signal(SIGFPE,  &crash_handler);
  std::signal(SIGILL,  &crash_handler);
#ifdef SIGBUS
  std::signal(SIGBUS,  &crash_handler);
#endif
  std::set_terminate(&terminate_handler);
}
#endif

void set_crash_log_path(std::string path) {
  g_log_path = std::move(path);
}

std::string crash_log_path() {
  return g_log_path;
}

}
