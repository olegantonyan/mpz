#ifndef LYRICS_LRCPARSER_H
#define LYRICS_LRCPARSER_H

#include <QString>

namespace Lyrics {
  class LrcParser {
  public:
    static bool looksLikeLrc(const QString &raw);
    static QString stripTimestamps(const QString &raw);
  };
}

#endif // LYRICS_LRCPARSER_H
