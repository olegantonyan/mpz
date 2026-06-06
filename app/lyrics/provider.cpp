#include "lyrics/provider.h"

namespace Lyrics {
  Provider::Provider(QObject *parent) : QObject(parent) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    nam.setTransferTimeout(10000);
#endif
  }

  QNetworkRequest Provider::makeRequest(const QUrl &url, bool browser_like_ua) const {
    QNetworkRequest req(url);
    if (browser_like_ua) {
      // NetEase/QQ endpoints reject obviously non-browser user agents.
      req.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"));
    } else {
      req.setHeader(QNetworkRequest::UserAgentHeader,
                    QString("mpz/%1 (https://github.com/olegantonyan/mpz)").arg(VERSION));
    }
    return req;
  }
}
