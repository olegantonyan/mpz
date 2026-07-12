#include "feedback_ui/feedbacksender.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QtGlobal>

namespace {
  const QString endpoint = QStringLiteral("https://us-central1-random-360814.cloudfunctions.net/mpz-feedback");
}

FeedbackSender::FeedbackSender(QObject *parent) : QObject(parent) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  nam.setTransferTimeout(30000);
#endif
}

void FeedbackSender::submit(const QString &text, const QString &author, const QString &sysinfo) {
  QJsonObject json;
  json["text"] = text;
  json["author"] = author;
  if (!sysinfo.isEmpty()) {
    json["sysinfo"] = sysinfo;
  }

  QUrl url(endpoint);
#ifdef DISABLE_HTTPS
  url.setScheme("http");
#endif
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QNetworkReply *reply = nam.post(request, QJsonDocument(json).toJson(QJsonDocument::Compact));
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    const bool ok = reply->error() == QNetworkReply::NoError;
    emit finished(ok, ok ? QString() : reply->errorString());
  });
}
