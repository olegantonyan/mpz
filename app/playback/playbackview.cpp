#include "playbackview.h"

namespace Playback {
  View::View(const Controls &c, QObject *parent) : QObject(parent), controls(c) {
  }

  void View::play(const Track &track) {
    emit started(track);
  }

  void View::stop() {
    emit stopped();
  }
}
