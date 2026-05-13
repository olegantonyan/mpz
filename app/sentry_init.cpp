#include "sentry_init.h"

#include <sentry.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace mpz::sentry {

void init(const QString &dsn, const QString &release) {
  if (dsn.isEmpty()) {
    return;
  }
  sentry_options_t *opts = sentry_options_new();
  sentry_options_set_dsn(opts, dsn.toUtf8().constData());
  sentry_options_set_release(opts, QString("mpz@%1").arg(release).toUtf8().constData());

  const QString db = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/sentry-db";
  sentry_options_set_database_path(opts, db.toUtf8().constData());

  // crashpad_handler ships next to mpz on Windows, in libexec/mpz on Linux.
  // For Qt Creator/dev builds where it's neither, fall back to the build-time
  // path injected by CMake via MPZ_CRASHPAD_HANDLER_BUILD_PATH.
  const QString app_dir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
  const QString installed = app_dir + "/crashpad_handler.exe";
#else
  const QString installed = QDir(app_dir + "/../libexec/mpz").absoluteFilePath("crashpad_handler");
#endif
  const QString handler = QFileInfo::exists(installed) ? installed : QString(MPZ_CRASHPAD_HANDLER_BUILD_PATH);
  sentry_options_set_handler_path(opts, handler.toUtf8().constData());

  sentry_init(opts);
}

void shutdown() {
  sentry_close();
}

}
