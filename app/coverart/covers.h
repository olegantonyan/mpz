#ifndef PLAYLIST_COVERS_H
#define PLAYLIST_COVERS_H

#include "coverart/embedded.h"
#include "coverart/foldercover.h"
#include "coverart/online/cache.h"
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

    // Built-in sources first, then any previously downloaded cover. Misses are
    // deliberately not cached: that is what lets a later download, or a cover
    // file dropped into the directory, be picked up by the next call.
    QString get(const QString& filepath, const QString &artist, const QString &album);

  private:
    explicit Covers(ModusOperandi &modus);

    QString keyByFilepath(const QString& filepath) const;
    FolderCover::Match bestLocalImage(const QString &dir) const;

    QHash <QString, QString> cache;
    CoverArt::Embedded embedded_covers;
    ModusOperandi &modus_operandi;
#ifdef ENABLE_MPD_SUPPORT
    CoverArt::Mpd mpd_covers;
#endif
  };
}

#endif // PLAYLIST_COVERS_H
