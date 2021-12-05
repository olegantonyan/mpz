#ifndef PLAYLIST_COVERS_H
#define PLAYLIST_COVERS_H

#include "coverart/embedded.h"

#include <QObject>
#include <QString>
#include <QHash>

namespace CoverArt {
  class Covers {
  public:
    static Covers &instance();

    QString get(const QString& filepath);

  private:
    explicit Covers();

    QString keyByFilepath(const QString& filepath) const;
    QString findCoverLocally(const QString &dir);

    QHash <QString, QString> cache;
    CoverArt::Embedded embedded_covers;
  };
}

#endif // PLAYLIST_COVERS_H
