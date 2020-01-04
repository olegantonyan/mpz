#include "randomtrail.h"

#include <QDebug>

namespace Playback {
  RandomTrail::RandomTrail(int max_size) : max_length(max_size) {
  }

  void RandomTrail::clear(){
    trail.clear();
  }

  void RandomTrail::add(quint64 track_uid) {
    if (trail.size() >= max_length) {
      clear();
    }
    //qDebug() << "push" << track_uid;
    trail.push(track_uid);
  }

  bool RandomTrail::exists(quint64 track_uid) const {
    return trail.contains(track_uid);
  }

  quint64 RandomTrail::prev() {
    if (trail.size() == 0) {
      return 0;
    }
    auto track_uid = trail.pop();
    //qDebug() << "pop" << track_uid;
    return track_uid;
  }
}
