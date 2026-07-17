#ifndef COVERART_ONLINE_PROVIDER_H
#define COVERART_ONLINE_PROVIDER_H

#include "coverart/online/albumquery.h"

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

namespace CoverArt {
  namespace Online {
    // Base class for online album cover providers. Exactly one of
    // found/notFound/failed is emitted per fetch(); unlike Lyrics::Provider the
    // contract is enforced rather than assumed, because a provider here issues
    // two requests (resolve a URL, then download it) and both replies can land
    // in the same event loop turn.
    class Provider : public QObject {
      Q_OBJECT
    public:
      enum class Ua {
        Default, // mpz/<version>
        Contact  // mpz/<version> ( <homepage> ) — MusicBrainz rejects anything less
      };

      explicit Provider(QObject *parent = nullptr);

      virtual void fetch(const AlbumQuery &query) = 0;

    signals:
      void found(const QByteArray &image, const QString &format);
      void notFound();
      void failed(const QString &message);

    protected:
      QNetworkRequest makeRequest(const QUrl &url, Ua ua = Ua::Default) const;
      // GETs url, validates the payload is a usable image, then emits.
      void downloadImage(const QUrl &url);

      void emitFound(const QByteArray &image, const QString &format);
      void emitNotFound();
      void emitFailed(const QString &message);

      QNetworkAccessManager nam;

    private:
      bool done = false;
    };
  }
}

#endif // COVERART_ONLINE_PROVIDER_H
