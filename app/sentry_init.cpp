#include "sentry_init.h"

#include <sentry.h>

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

  sentry_init(opts);
}

void shutdown() {
  sentry_close();
}

}
