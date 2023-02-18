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

  QList<SortingPreset> Global::sortPresets() const {
    QList<SortingPreset> result;

    auto raw = storage.get("sort_presets");
    if (raw.listType() != Config::Value::Map) {
      return result;
    }

    auto list = raw.get<QList<Config::Value> >();
    for (auto i : list) {
      auto map = i.get<QMap<QString, Config::Value>>();
      QString name = map.value("name").get<QString>();
      QString value = map.value("value").get<QString>();

      result << SortingPreset(name, value);
    }

    return result;
  }

  bool Global::saveSortPresets(const QList<SortingPreset> &arg) {
    QList<Config::Value> result;
    for (auto i : arg) {
      QMap<QString, Config::Value> mp;
      mp.insert("name", Config::Value(i.first));
      mp.insert("value", Config::Value(i.second));
      result << Config::Value(mp);
    }
    return storage.set("sort_presets", Config::Value(result));
  }

  QString Global::language() const {
    return storage.get("language").get<QString>();
  }

  int Global::ipcPort() const {
    return storage.get("single_instance_ipc_port").get<int>();
  }

  bool Global::saveIpcPort(int arg) {
    return storage.set("single_instance_ipc_port", Config::Value(arg));
  }

  bool Global::singleInstance() const {
    return storage.get("single_instance").get<bool>();
  }

  int Global::playbackLogSize() const {
    return storage.get("playback_log_size").get<int>();
  }

  PlaylistUi::ColumnsConfig Global::columnsConfig() const {
    auto raw = storage.get("columns_config").get<QList<Config::Value>>();
    return PlaylistUi::ColumnsConfig::deserialize(raw);
  }

  bool Global::saveColumnsConfig(const PlaylistUi::ColumnsConfig &arg) {
    return storage.set("columns_config", arg.serialize());
  }

  bool Global::inhibitSleepWhilePlaying() const {
    return storage.get("inhibit_sleep_while_playing").get<bool>();
  }

  int Global::playlistRowHeight() const {
    return storage.get("playlist_row_height").get<int>();
  }
}
