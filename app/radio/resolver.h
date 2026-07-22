#ifndef RADIO_RESOLVER_H
#define RADIO_RESOLVER_H

#include <QByteArray>
#include <QString>

namespace Radio {
  // A raw stream url passes through; a .pls/.m3u (local file or remote url) is
  // read/fetched and its first stream url extracted. Empty + *error on failure.
  QString resolveStreamUrl(const QString &input, QString *error = nullptr);

  QString firstStreamUrl(const QByteArray &playlist_body);

  bool looksLikePlaylist(const QString &path_or_url);

  // Writes each out-param only when the url path yields a value ("foo-256-mp3").
  void guessStreamFormat(const QString &url, QString *codec, quint16 *bitrate);
}

#endif // RADIO_RESOLVER_H
