#include "pendingnext.h"

namespace Playback {
  void PendingNext::set(quint64 boundary_track_uid, const Track &next) {
    has_pending = true;
    boundary_uid = boundary_track_uid;
    next_track = next;
  }

  void PendingNext::clear() {
    has_pending = false;
    boundary_uid = 0;
    next_track = Track();
  }

  bool PendingNext::validFor(quint64 playing_track_uid) const {
    return has_pending && boundary_uid == playing_track_uid;
  }

  Track PendingNext::take() {
    Track t = next_track;
    clear();
    return t;
  }

  quint64 PendingNext::boundaryUid() const {
    return boundary_uid;
  }
}
