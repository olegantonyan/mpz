#ifndef COVERART_MPD_H
#define COVERART_MPD_H

#include "mpd_client/client.h"

#include <QString>
#include <QTemporaryFile>
#include <QHash>

namespace CoverArt {
  struct Picture {
    uint32_t type = 0;
    QString mime;
    QString description;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
    uint32_t colors = 0;
    QByteArray data;
    bool isFrontCover() { return type == 3; }
  };

  class Mpd {
  public:
    Mpd(MpdClient::Client &cl);

    QString get(const QString &filepath);

  private:
    QString temp_dir;
    MpdClient::Client &client;
    QHash <QByteArray, std::shared_ptr<QTemporaryFile>> files_cache;

    std::shared_ptr<QTemporaryFile> create_tempfile();
    QByteArray image_hash(const QByteArray &ba) const;
    QVector<Picture> parsePictures(const QByteArray &src);
    QByteArray fetch(const QString &filepath);
  };
}

#endif // COVERART_MPD_H
