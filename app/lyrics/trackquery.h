#ifndef LYRICS_TRACKQUERY_H
#define LYRICS_TRACKQUERY_H

#include <QString>

namespace Lyrics {
  struct TrackQuery {
    QString artist;
    QString title;
    QString album;
    int duration_seconds = 0; // 0 = unknown
  };
}

#endif // LYRICS_TRACKQUERY_H
