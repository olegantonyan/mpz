#include "playbackview.h"

namespace Playback {
  PlaybackView::PlaybackView(const Controls &c, QObject *parent) : QObject(parent), controls(c) {
  }

  void PlaybackView::play(const Track &track) {
    emit started(track);
  }

  void PlaybackView::stop() {
    emit stopped();
  }
}
