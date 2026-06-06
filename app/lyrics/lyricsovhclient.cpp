#include "lyrics/lyricsovhclient.h"
#include "lyrics/textmatch.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>

namespace Lyrics {
  LyricsOvhClient::LyricsOvhClient(QObject *parent) : Provider(parent) {
  }

  void LyricsOvhClient::fetch(const TrackQuery &q) {
    attempts.clear();
    next_attempt = 0;
    last_error.clear();

    const QString artist = TextMatch::normalizeArtist(q.artist);
    const QString title = TextMatch::normalizeTitle(q.title);
    const QString primary = TextMatch::normalizeArtist(TextMatch::primaryArtist(q.artist));

    if (artist.isEmpty() || title.isEmpty()) { // both path segments are mandatory
      emit notFound();
      return;
    }
    attempts.append({artist, title});
    if (primary != artist) {
      attempts.append({primary, title});
    }
    runNext();
  }

  void LyricsOvhClient::runNext() {
    if (next_attempt >= attempts.size()) {
      if (!last_error.isEmpty()) {
        emit failed(last_error);
      } else {
        emit notFound();
      }
      return;
    }
    const Attempt attempt = attempts.at(next_attempt++);
    const QUrl url(QStringLiteral("https://api.lyrics.ovh/v1/%1/%2")
                     .arg(QString::fromLatin1(QUrl::toPercentEncoding(attempt.artist)),
                          QString::fromLatin1(QUrl::toPercentEncoding(attempt.title))));
    auto *reply = nam.get(makeRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply); });
  }

  void LyricsOvhClient::handleReply(QNetworkReply *reply) {
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError
        && reply->error() != QNetworkReply::ContentNotFoundError && status != 404) {
      last_error = reply->errorString();
      runNext();
      return;
    }
    QJsonParseError perr;
    const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
      runNext();
      return;
    }
    const QString lyrics = doc.object().value("lyrics").toString().trimmed();
    if (lyrics.isEmpty()) {
      runNext();
      return;
    }
    emit found(lyrics);
  }
}
