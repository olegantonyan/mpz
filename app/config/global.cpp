#include "global.h"

#include <QDebug>

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

  bool Global::minimizeToTray() const {
    return storage.get("minimize_to_tray").get<bool>();
  }

  void Global::saveMinimizeToTray(bool arg) {
    storage.set("minimize_to_tray", Config::Value(arg));
  }

  QList<QPair<QString, QString> > Global::sortPresets() const {
    QList<QPair<QString, QString> > result;
    result << QPair<QString, QString>("Artist / Album / -Title", "Artist / Album / -Title");
    result << QPair<QString, QString>("year / title ", " year/    tiTle");
    result << QPair<QString, QString>("-title", "-title");

    auto raw = storage.get("sort_presets");
    if (raw.listType() != Config::Value::Map) {
      return result;
    }

    return result;
  }

  /*bool Global::saveSortPresets(const QMap<QString, QString> &arg) {

  }*/
}
