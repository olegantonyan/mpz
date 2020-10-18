#ifndef SORTER_H
#define SORTER_H

#include "track.h"

#include <QString>

namespace Playlist {
  class Sorter {
  public:
    explicit Sorter(const QString &criteria = "%DATE% - %ALBUM% - %DISCNUMBER% - %TRACKNUMBER% - %TITLE%");

  bool condition(const Track &t1, const Track &t2) const;

  private:
    QString criteria;
  };
}

#endif // SORTER_H
