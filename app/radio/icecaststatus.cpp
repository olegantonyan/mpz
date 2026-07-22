#include "radio/icecaststatus.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace Radio {
  namespace {
    const int kGraceMs = 5000;
    const int kPollIntervalMs = 15000;
    const int kRequestTimeoutMs = 10000;

    QString titleFromSource(const QJsonObject &source) {
      const auto title = source.value(QStringLiteral("title")).toString().trimmed();
      if (!title.isEmpty()) {
        return title;
      }
      const auto display = source.value(QStringLiteral("display-title")).toString().trimmed();
      if (!display.isEmpty()) {
        return display;
      }
      // Icecast 2.5 parks the source client's ICY title here and leaves "title" null
      const auto icy = source.value("metadata").toObject().value("x_icy_title").toString().trimmed();
      if (!icy.isEmpty()) {
        return icy;
      }
      const auto list = source.value("playlist").toObject().value("trackList").toArray();
      if (list.isEmpty()) {
        return QString();
      }
      return list.at(list.size() - 1).toObject().value("title").toString().trimmed();
    }
  }

  QString nowPlayingFromStatusJson(const QByteArray &body, const QString &mount, bool *source_found) {
    if (source_found) {
      *source_found = false;
    }
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
      return QString();
    }
    const auto source = doc.object().value("icestats").toObject().value("source");

    if (source.isObject()) {
      if (source_found) {
        *source_found = true;
      }
      return titleFromSource(source.toObject());
    }
    if (!source.isArray()) {
      return QString();
    }

    const auto sources = source.toArray();
    for (const auto &i : sources) {
      const auto obj = i.toObject();
      if (!mount.isEmpty() && QUrl(obj.value("listenurl").toString()).path() == mount) {
        if (source_found) {
          *source_found = true;
        }
        return titleFromSource(obj);
      }
    }
    if (sources.size() == 1) {
      if (source_found) {
        *source_found = true;
      }
      return titleFromSource(sources.at(0).toObject());
    }
    return QString();
  }

  QUrl statusJsonUrl(const QUrl &stream_url) {
    QUrl u;
    u.setScheme(stream_url.scheme());
    u.setHost(stream_url.host());
    if (stream_url.port() > 0) {
      u.setPort(stream_url.port());
    }
    u.setPath(QStringLiteral("/status-json.xsl"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("mount"), stream_url.path());
    u.setQuery(q);
    return u;
  }

  IcecastStatus::IcecastStatus(QObject *parent) :
    QObject(parent),
    current(nullptr),
    retried_without_mount(false) {
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &IcecastStatus::poll);
    request_timeout.setSingleShot(true);
    connect(&request_timeout, &QTimer::timeout, this, [this]() {
      if (current) {
        current->abort();
      }
    });
  }

  void IcecastStatus::start(const QUrl &stream_url) {
    stop();
    if (stream_url.scheme() != "http" && stream_url.scheme() != "https") {
      return;
    }
    status_url = statusJsonUrl(stream_url);
    mount = stream_url.path();
    if (!stream_url.userName().isEmpty() && !stream_url.password().isEmpty()) {
      const auto concatenated = stream_url.userName() + ":" + stream_url.password();
      auth_header = "Basic " + concatenated.toLocal8Bit().toBase64();
    }
    timer.start(kGraceMs);
  }

  void IcecastStatus::stop() {
    timer.stop();
    request_timeout.stop();
    if (current) {
      current->disconnect(this);
      current->abort();
      current->deleteLater();
      current = nullptr;
    }
    status_url.clear();
    mount.clear();
    auth_header.clear();
    last_emitted.clear();
    retried_without_mount = false;
  }

  void IcecastStatus::poll() {
    if (status_url.isEmpty() || current) {
      return;
    }
    QUrl u = status_url;
    if (retried_without_mount) {
      u.setQuery(QString());
    }
    QNetworkRequest req{u};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QString("mpz/%1").arg(QCoreApplication::applicationVersion()));
    if (!auth_header.isEmpty()) {
      req.setRawHeader("Authorization", auth_header);
    }
    current = nam.get(req);
    connect(current, &QNetworkReply::finished, this, &IcecastStatus::handleReply);
    request_timeout.start(kRequestTimeoutMs);
  }

  void IcecastStatus::handleReply() {
    request_timeout.stop();
    auto *reply = current;
    current = nullptr;
    if (!reply) {
      return;
    }
    reply->deleteLater();
    if (status_url.isEmpty()) {
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      timer.start(kPollIntervalMs);
      return;
    }

    bool source_found = false;
    const auto np = nowPlayingFromStatusJson(reply->readAll(), mount, &source_found);

    if (!source_found && !retried_without_mount) {
      retried_without_mount = true;
      timer.start(0);
      return;
    }

    timer.start(kPollIntervalMs);
    if (np.isEmpty() || np == last_emitted) {
      return;
    }
    last_emitted = np;
    emit nowPlaying(np);
  }
}
