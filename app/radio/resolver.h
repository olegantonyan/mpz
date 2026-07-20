#ifndef RADIO_RESOLVER_H
#define RADIO_RESOLVER_H

#include <QByteArray>
#include <QString>

namespace Radio {
  // Turns whatever the user typed into a directly-playable stream url.
  //
  //  - a raw http(s) stream url is returned unchanged;
  //  - a local .pls/.m3u/.m3u8 file is read and its first stream url extracted;
  //  - a remote .pls/.m3u/.m3u8 url is fetched (blocking, with a timeout) and
  //    its first stream url extracted.
  //
  // Returns an empty string and sets *error on failure.
  QString resolveStreamUrl(const QString &input, QString *error = nullptr);

  // First stream url inside a .pls (FileN=...) or .m3u (first non-comment http
  // line) body. Pure and network-free; empty if none found.
  QString firstStreamUrl(const QByteArray &playlist_body);

  bool looksLikePlaylist(const QString &path_or_url);
}

#endif // RADIO_RESOLVER_H
