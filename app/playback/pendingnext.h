#ifndef PENDINGNEXT_H
#define PENDINGNEXT_H

#include "track.h"

#include <QtGlobal>

namespace Playback {
  class PendingNext {
  public:
    void set(quint64 boundary_track_uid, const Track &next);
    void clear();
    bool validFor(quint64 playing_track_uid) const;
    Track take();
    quint64 boundaryUid() const;

  private:
    bool has_pending = false;
    quint64 boundary_uid = 0;
    Track next_track;
  };
}

#endif // PENDINGNEXT_H
