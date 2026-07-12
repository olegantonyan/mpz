#include "crash_report/crashreport.h"
#include "crash_report/crashlog_format.h"

CrashEntry lastCrash(const QString &log) {
  CrashEntry entry;

  const int begin = log.lastIndexOf(QLatin1String(kCrashBegin));
  if (begin < 0) {
    return entry;
  }
  entry.text = log.mid(begin).trimmed();

  const int time = entry.text.indexOf(QLatin1String(kCrashTimeLabel));
  if (time >= 0) {
    const int value = time + static_cast<int>(qstrlen(kCrashTimeLabel));
    const int eol = entry.text.indexOf(QLatin1Char('\n'), value);
    entry.id = entry.text.mid(value, eol < 0 ? -1 : eol - value).trimmed();
  }

  entry.valid = !entry.id.isEmpty();
  return entry;
}
