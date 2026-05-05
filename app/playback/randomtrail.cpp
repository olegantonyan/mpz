#include "randomtrail.h"

namespace Playback {
  RandomTrail::RandomTrail(int max_size) : cursor(-1), max_length(max_size) {
  }

  void RandomTrail::clear() {
    trail.clear();
    cursor = -1;
  }

  void RandomTrail::add(quint64 track_uid) {
    if (cursor >= 0 && cursor < trail.size() && trail.at(cursor) == track_uid) {
      return;
    }
    if (cursor + 1 < trail.size() && trail.at(cursor + 1) == track_uid) {
      cursor++;
      return;
    }
    if (cursor + 1 < trail.size()) {
      trail.resize(cursor + 1);
    }
    trail.append(track_uid);
    cursor = trail.size() - 1;
    while (trail.size() > max_length) {
      trail.removeFirst();
      cursor--;
    }
  }

  bool RandomTrail::recentlyPlayed(quint64 track_uid, int window) const {
    if (window <= 0 || cursor < 0) {
      return false;
    }
    int from = cursor - window + 1;
    if (from < 0) {
      from = 0;
    }
    for (int i = from; i <= cursor && i < trail.size(); i++) {
      if (trail.at(i) == track_uid) {
        return true;
      }
    }
    return false;
  }

  quint64 RandomTrail::goPrev() {
    if (cursor <= 0) {
      return 0;
    }
    cursor--;
    return trail.at(cursor);
  }
}
