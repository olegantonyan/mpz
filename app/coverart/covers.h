#ifndef PLAYLIST_COVERS_H
#define PLAYLIST_COVERS_H

#include "coverart/embedded.h"
#include "modusoperandi.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "coverart/mpd.h"
#endif

#include <QObject>
#include <QString>
#include <QHash>

namespace CoverArt {
  class Covers {
  public:
    static Covers &instance(ModusOperandi &modus);
    static Covers &instance();

    QString get(const QString& filepath);

  private:
    explicit Covers(ModusOperandi &modus);

    QString keyByFilepath(const QString& filepath) const;
    QString findCoverLocally(const QString &dir);

    QHash <QString, QString> cache;
    CoverArt::Embedded embedded_covers;
    ModusOperandi &modus_operandi;
#ifdef ENABLE_MPD_SUPPORT
    CoverArt::Mpd mpd_covers;
#endif
  };
}

#endif // PLAYLIST_COVERS_H
