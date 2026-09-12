#ifndef CRASHLOG_FORMAT_H
#define CRASHLOG_FORMAT_H

constexpr const char *kCrashBegin = "======== MPZ CRASH BEGIN ========";
constexpr const char *kCrashEnd = "======== MPZ CRASH END ========";
constexpr const char *kCrashTimeLabel = "Time: ";
constexpr const char *kCrashReasonLabel = "Reason: ";
constexpr const char *kCrashPhaseLabel = "Phase: ";
constexpr const char *kCrashThreadLabel = "Thread: ";
constexpr const char *kCrashSignalCodeLabel = "Signal code: ";
constexpr const char *kCrashFaultAddressLabel = "Fault address: ";

#endif // CRASHLOG_FORMAT_H
