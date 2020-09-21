#include "global.h"

namespace Config {
  Global::Global() : storage("global.yml") {
  }

  bool Global::sync() {
    return storage.save();
  }

  bool Global::playbackFollowCursor() const {
    return storage.get("playback_follows_cursor").get<bool>();
  }

  void Global::savePlaybackFollowCursor(bool arg) {
    storage.set("playback_follows_cursor", Config::Value(arg));
  }

  QString Global::playbackOrder() const {
    return storage.get("playback_order").get<QString>();
  }

  void Global::savePlaybackOrder(const QString &arg) {
    storage.set("playback_order", Config::Value(arg));
  }

  bool Global::trayIconEnabled() const {
    return storage.get("tray_icon_enabled").get<bool>();
  }

  void Global::saveTrayIconEnabled(bool arg) {
    storage.set("tray_icon_enabled", Config::Value(arg));
  }

  int Global::streamBufferSize() const {
    return storage.get("stream_buffer_size").get<int>();
  }

  void Global::saveStreamBufferSize(int arg) {
    storage.set("stream_buffer_size", Config::Value(arg));
  }
}
