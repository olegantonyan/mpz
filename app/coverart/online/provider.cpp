#include "coverart/online/provider.h"

#include <QApplication>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkReply>

namespace CoverArt {
  namespace Online {
    namespace {
      const qint64 MAX_IMAGE_BYTES = 10 * 1024 * 1024;
      const int MIN_IMAGE_SIDE = 100; // rejects 1x1 spacers and "no artwork" stubs
    }

    Provider::Provider(QObject *parent) : QObject(parent) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
      nam.setTransferTimeout(10000);
#endif
      // Cover Art Archive answers front-500 with a redirect to archive.org. Qt6
      // follows redirects by default, Qt5 does not, and mpz builds both.
      nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    }

    QNetworkRequest Provider::makeRequest(const QUrl &url, Ua ua) const {
      QNetworkRequest req(url);
      if (ua == Ua::Contact) {
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      QString("mpz/%1 ( https://github.com/olegantonyan/mpz )").arg(qApp->applicationVersion()));
      } else {
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      QString("mpz/%1").arg(qApp->applicationVersion()));
      }
      return req;
    }

    void Provider::downloadImage(const QUrl &url) {
      auto *reply = nam.get(makeRequest(url));
      connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::ContentNotFoundError || status == 404) {
          emitNotFound();
          return;
        }
        if (reply->error() != QNetworkReply::NoError) {
          emitFailed(reply->errorString());
          return;
        }
        const QByteArray body = reply->readAll();
        if (body.isEmpty() || body.size() > MAX_IMAGE_BYTES) {
          emitNotFound();
          return;
        }
        QBuffer buffer;
        buffer.setData(body);
        if (!buffer.open(QIODevice::ReadOnly)) {
          emitNotFound();
          return;
        }
        QImageReader reader(&buffer);
        const QString format = QString::fromLatin1(reader.format()).toLower();
        const QSize size = reader.size();
        if (format.isEmpty() || !size.isValid() ||
            size.width() < MIN_IMAGE_SIDE || size.height() < MIN_IMAGE_SIDE) {
          emitNotFound();
          return;
        }
        emitFound(body, format);
      });
    }

    void Provider::emitFound(const QByteArray &image, const QString &format) {
      if (done) {
        return;
      }
      done = true;
      emit found(image, format);
    }

    void Provider::emitNotFound() {
      if (done) {
        return;
      }
      done = true;
      emit notFound();
    }

    void Provider::emitFailed(const QString &message) {
      if (done) {
        return;
      }
      done = true;
      emit failed(message);
    }
  }
}
