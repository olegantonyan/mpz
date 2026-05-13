#pragma once

#include <QString>

namespace mpz::sentry {
void init(const QString &dsn, const QString &release);
void shutdown();
}
