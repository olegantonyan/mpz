#include "randomtrail.h"

#include <QDebug>

namespace Playback {
  RandomTrail::RandomTrail(int max_size) : max_length(max_size) {
    trail.clear();
  }

  void RandomTrail::add(quint64 track_uid) {
    if (trail.size() >= max_length) {
      trail.remove(0);
    }
    trail.append(track_uid);
  }

  bool RandomTrail::exists(quint64 track_uid) const {
    for (auto i : trail) {
      if (i == track_uid) {
        return true;
      }
    }
    return false;
  }

  quint64 RandomTrail::prev(quint64 track_uid) const {
    if (trail.size() == 0) {
      return 0;
    }
    for (int i = 0; i < trail.size(); i++) {
      if (trail.at(i) == track_uid && i - 1 >= 0) {
        return trail.at(i - 1);
      }
    }
    return trail.first();
  }
}
