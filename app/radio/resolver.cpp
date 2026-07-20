#include "radio/resolver.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextStream>
#include <QTimer>
#include <QUrl>

namespace Radio {
  namespace {
    const int kFetchTimeoutMs = 8000;

    bool isHttpUrl(const QString &s) {
      const QUrl u(s);
      return u.scheme() == "http" || u.scheme() == "https";
    }

    // A single .pls/.m3u line -> the stream url it points at, or empty.
    QString streamFromLine(const QString &line) {
      if (line.startsWith("http", Qt::CaseInsensitive)) { // m3u entry
        return line;
      }
      if (line.startsWith("File", Qt::CaseInsensitive)) { // pls "FileN=url"
        int i = 4;
        while (i < line.size() && line.at(i).isDigit()) {
          i++;
        }
        if (i > 4 && i < line.size() && line.at(i) == '=') {
          const auto value = line.mid(i + 1).trimmed();
          if (value.startsWith("http", Qt::CaseInsensitive)) {
            return value;
          }
        }
      }
      return QString();
    }
  }

  bool looksLikePlaylist(const QString &path_or_url) {
    const auto path = QUrl(path_or_url).path().isEmpty() ? path_or_url : QUrl(path_or_url).path();
    return path.endsWith(".pls", Qt::CaseInsensitive)
        || path.endsWith(".m3u", Qt::CaseInsensitive)
        || path.endsWith(".m3u8", Qt::CaseInsensitive);
  }

  QString firstStreamUrl(const QByteArray &playlist_body) {
    QTextStream in(playlist_body);
    while (!in.atEnd()) {
      const auto line = in.readLine().trimmed();
      if (line.isEmpty() || line.startsWith('#')) {
        continue;
      }
      const auto url = streamFromLine(line);
      if (!url.isEmpty()) {
        return url;
      }
    }
    return QString();
  }

  QString resolveStreamUrl(const QString &input, QString *error) {
    const auto set_error = [error](const QString &message) {
      if (error) {
        *error = message;
      }
      return QString();
    };
    if (error) {
      error->clear();
    }

    const auto trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
      return set_error(QObject::tr("empty url"));
    }

    // Raw stream url -> use as-is.
    if (!looksLikePlaylist(trimmed)) {
      if (!isHttpUrl(trimmed)) {
        return set_error(QObject::tr("not an http or https url: %1").arg(trimmed));
      }
      return trimmed;
    }

    // Local playlist file.
    if (!isHttpUrl(trimmed)) {
      const auto local = QUrl(trimmed).isLocalFile() ? QUrl(trimmed).toLocalFile() : trimmed;
      QFile f(local);
      if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return set_error(QObject::tr("cannot read %1").arg(local));
      }
      const auto url = firstStreamUrl(f.readAll());
      return url.isEmpty() ? set_error(QObject::tr("no stream url found in %1").arg(local)) : url;
    }

    // Remote playlist url -> fetch and extract.
    QNetworkAccessManager nam;
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkRequest req{QUrl(trimmed)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("mpz"));

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    auto *reply = nam.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(kFetchTimeoutMs);
    loop.exec();

    if (!timeout.isActive()) { // timer fired first
      reply->abort();
      reply->deleteLater();
      return set_error(QObject::tr("timed out fetching %1").arg(trimmed));
    }
    timeout.stop();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      return set_error(reply->errorString());
    }
    const auto url = firstStreamUrl(reply->readAll());
    return url.isEmpty() ? set_error(QObject::tr("no stream url found in %1").arg(trimmed)) : url;
  }
}
