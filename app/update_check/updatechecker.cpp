#include "update_check/updatechecker.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QVersionNumber>
#include <QtGlobal>

namespace {
  const QString latest_release_url = QStringLiteral("https://api.github.com/repos/olegantonyan/mpz/releases/latest");
  const qint64 throttle_seconds = 24 * 60 * 60;

  QVersionNumber parseVersion(const QString &raw) {
    QString s = raw.trimmed();
    if (s.startsWith('v') || s.startsWith('V')) {
      s = s.mid(1);
    }
    return QVersionNumber::fromString(s); // stops at the first non-numeric char, so a "2.0.12 [next…]" suffix is ignored
  }
}

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  nam.setTransferTimeout(10000);
#endif
}

void UpdateChecker::check() {
  qint64 last_check = 0;
  QString cached_version;
  QString cached_url;
  if (readCache(last_check, cached_version, cached_url) &&
      QDateTime::currentSecsSinceEpoch() - last_check < throttle_seconds) {
    emitIfNewer(cached_version, cached_url);
    return;
  }
  fetch();
}

void UpdateChecker::fetch() {
  QNetworkRequest req{QUrl(latest_release_url)};
  req.setHeader(QNetworkRequest::UserAgentHeader, QString("mpz/%1").arg(qApp->applicationVersion()));
  req.setRawHeader("Accept", "application/vnd.github+json");

  auto *reply = nam.get(req);
  connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply); });
}

void UpdateChecker::handleReply(QNetworkReply *reply) {
  reply->deleteLater();

  const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (reply->error() != QNetworkReply::NoError || status != 200) {
    qWarning() << "update check failed:" << reply->errorString() << "http" << status;
    return;
  }

  QJsonParseError perr;
  const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
  if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
    qWarning() << "update check: invalid response from github";
    return;
  }

  const auto obj = doc.object();
  const QString tag = obj.value("tag_name").toString();
  const QString url = obj.value("html_url").toString();
  if (tag.isEmpty() || url.isEmpty()) {
    qWarning() << "update check: missing tag_name/html_url";
    return;
  }

  writeCache(tag, url);
  emitIfNewer(tag, url);
}

void UpdateChecker::emitIfNewer(const QString &version, const QString &url) {
  if (version.isEmpty() || url.isEmpty()) {
    return;
  }
  if (isNewer(version, qApp->applicationVersion())) {
    emit updateAvailable(version, url);
  }
}

bool UpdateChecker::isNewer(const QString &remoteTag, const QString &localVersion) {
  const QVersionNumber remote = parseVersion(remoteTag);
  const QVersionNumber local = parseVersion(localVersion);
  if (remote.isNull() || local.isNull()) {
    return false;
  }
  return remote > local;
}

QString UpdateChecker::cachePath() const {
  return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + "mpz_update_check.json";
}

bool UpdateChecker::readCache(qint64 &last_check, QString &version, QString &url) const {
  QFile f(cachePath());
  if (!f.open(QIODevice::ReadOnly)) {
    return false;
  }
  const auto doc = QJsonDocument::fromJson(f.readAll());
  if (!doc.isObject()) {
    return false;
  }
  const auto obj = doc.object();
  if (!obj.contains("last_check")) {
    return false;
  }
  last_check = static_cast<qint64>(obj.value("last_check").toDouble());
  version = obj.value("version").toString();
  url = obj.value("url").toString();
  return true;
}

void UpdateChecker::writeCache(const QString &version, const QString &url) {
  QJsonObject obj;
  obj.insert("last_check", static_cast<double>(QDateTime::currentSecsSinceEpoch()));
  obj.insert("version", version);
  obj.insert("url", url);

  const QString path = cachePath();
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "update check: cannot write cache" << path;
    return;
  }
  f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
