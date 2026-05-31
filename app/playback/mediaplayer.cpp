#include "mediaplayer.h"

namespace Playback {
  MediaPlayer::MediaPlayer(QObject *parent) : QObject(parent) {}

  // Default next/prev simply ask the dispatcher for the adjacent track. Backends
  // that need extra bookkeeping (Qtmm's EOF latch, MPD's daemon commands)
  // override these.
  void MediaPlayer::next() {
    emit nextRequested();
  }

  void MediaPlayer::prev() {
    emit prevRequested();
  }

  void MediaPlayer::setOutputDevice(QByteArray) {
    // No-op default; backends with output-device control override this.
  }
}
