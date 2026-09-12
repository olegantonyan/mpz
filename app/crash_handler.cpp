#include "crash_handler.h"
#include "crash_report/crashlog_format.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

#include <atomic>
#include <cinttypes>
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
#else
#include <pthread.h>
#endif

namespace mpz {

namespace {

const cpptrace::formatter g_formatter =
    cpptrace::formatter{}
        .addresses(cpptrace::formatter::address_mode::object)
        .transform([](cpptrace::stacktrace_frame frame) {
          if (frame.object_address == 0) frame.object_address = frame.raw_address;
          return frame;
        });

std::string g_log_path;
std::string g_system_info;
std::atomic<const char *> g_phase{"starting"};

void thread_name(char *buf, std::size_t size) {
#ifdef _WIN32
  std::snprintf(buf, size, "%lu", GetCurrentThreadId());
#else
  if (pthread_getname_np(pthread_self(), buf, size) != 0) {
    std::snprintf(buf, size, "?");
  }
#endif
}

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

void write_trace(const cpptrace::stacktrace &trace, const char *reason, const std::string &details = {}) {
  std::fprintf(stderr, "\nFatal: %s\n%s\n", reason, details.c_str());
  g_formatter.print(std::cerr, trace);

  if (g_log_path.empty()) return;
  std::ofstream ofs(g_log_path, std::ios::app);
  if (!ofs) return;
  char tname[24];
  thread_name(tname, sizeof(tname));

  ofs << "\n" << kCrashBegin << "\n";
  ofs << kCrashTimeLabel << std::time(nullptr) << "\n";
  ofs << kCrashReasonLabel << reason << "\n";
  ofs << kCrashPhaseLabel << g_phase.load(std::memory_order_relaxed) << "\n";
  ofs << kCrashThreadLabel << tname << "\n";
  ofs << details;
  if (!g_system_info.empty()) ofs << g_system_info << "\n\n";
  g_formatter.print(ofs, trace);
  ofs << "\n" << kCrashEnd << "\n";
}

#ifdef _WIN32
[[noreturn]] void crash_handler(int signum) {
  // Restore default disposition so a second fault while we're tracing aborts hard instead of recursing.
  std::signal(signum, SIG_DFL);

  // async-signal-unsafe but pragmatic — same trade-off as DeathHandler. A desktop app crashing
  // once is fine to take a few hundred ms in the handler if it gets a usable trace out of it.
  char reason[32];
  std::snprintf(reason, sizeof(reason), "%s (%d)", signal_name(signum), signum);
  write_trace(cpptrace::generate_trace(/*skip=*/1), reason);

  std::_Exit(128 + signum);
}
#else
bool fault_address_is_set(int signum, int si_code) {
  if (si_code <= 0) return false;
  switch (signum) {
    case SIGSEGV:
    case SIGILL:
    case SIGFPE:
#ifdef SIGBUS
    case SIGBUS:
#endif
      return true;
    default: return false;
  }
}

std::string signal_details(int signum, const siginfo_t *info) {
  if (info == nullptr) return {};

  std::string details = std::string(kCrashSignalCodeLabel) + std::to_string(info->si_code) + "\n";
  if (!fault_address_is_set(signum, info->si_code)) return details;

  const auto address = reinterpret_cast<cpptrace::frame_ptr>(info->si_addr);
  char hex[32];
  std::snprintf(hex, sizeof(hex), "0x%016" PRIxPTR, address);
  details += kCrashFaultAddressLabel;
  details += hex;

  cpptrace::safe_object_frame object{};
  if (cpptrace::can_get_safe_object_frame()) cpptrace::get_safe_object_frame(address, &object);
  if (object.object_path[0] != 0) {
    std::snprintf(hex, sizeof(hex), "+0x%" PRIxPTR, object.address_relative_to_object_start);
    details += " (";
    details += object.object_path;
    details += hex;
    details += ")";
  }
  details += "\n";
  return details;
}

[[noreturn]] void crash_handler(int signum, siginfo_t *info, void *) {
  // Restore default disposition so a second fault while we're tracing aborts hard instead of recursing.
  std::signal(signum, SIG_DFL);

  // async-signal-unsafe but pragmatic — same trade-off as DeathHandler. A desktop app crashing
  // once is fine to take a few hundred ms in the handler if it gets a usable trace out of it.
  char reason[32];
  std::snprintf(reason, sizeof(reason), "%s (%d)", signal_name(signum), signum);
  write_trace(cpptrace::generate_trace(/*skip=*/1), reason, signal_details(signum, info));

  std::_Exit(128 + signum);
}
#endif

[[noreturn]] void terminate_handler() {
  std::signal(SIGABRT, SIG_DFL);
  write_trace(cpptrace::generate_trace(/*skip=*/1), "std::terminate");
  std::abort();
}

#ifdef _WIN32
// A real access violation is delivered as an SEH exception, not a C signal, so the std::signal handlers
// below never fire for it on Windows. This top-level filter runs before stack unwinding, so generate_trace()
// still sees the faulting frames (rooted at the filter itself). cpptrace installs no such filter itself.
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
  struct sigaction action{};
  action.sa_sigaction = &crash_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;

  sigaction(SIGSEGV, &action, nullptr);
  sigaction(SIGABRT, &action, nullptr);
  sigaction(SIGFPE,  &action, nullptr);
  sigaction(SIGILL,  &action, nullptr);
#ifdef SIGBUS
  sigaction(SIGBUS,  &action, nullptr);
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

void set_system_info(std::string info) {
  g_system_info = std::move(info);
}

void set_crash_phase(const char *phase) {
  g_phase.store(phase, std::memory_order_relaxed);
}

}
