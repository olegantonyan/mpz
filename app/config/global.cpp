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

  bool Global::disableAutoUpdateCheck() const {
    return storage.get("disable_auto_update_check").get<bool>();
  }

  void Global::saveDisableAutoUpdateCheck(bool arg) {
    storage.set("disable_auto_update_check", Config::Value(arg));
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
    for (const auto &i : std::as_const(list)) {
      auto map = i.get<QMap<QString, Config::Value>>();
      QString name = map.value("name").get<QString>();
      QString value = map.value("value").get<QString>();

      result << SortingPreset(name, value);
    }

    return result;
  }

  bool Global::saveSortPresets(const QList<SortingPreset> &arg) {
    QList<Config::Value> result;
    for (const auto &i : std::as_const(arg)) {
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

  void Global::saveLanguage(const QString &arg) {
    storage.set("language", Config::Value(arg));
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

  void Global::saveSingleInstance(bool arg) {
    storage.set("single_instance", Config::Value(arg));
  }

  int Global::playbackLogSize() const {
    return storage.get("playback_log_size").get<int>();
  }

  void Global::savePlaybackLogSize(int arg) {
    storage.set("playback_log_size", Config::Value(arg));
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

  void Global::saveInhibitSleepWhilePlaying(bool arg) {
    storage.set("inhibit_sleep_while_playing", Config::Value(arg));
  }

  bool Global::stopWhenTrackRemoved() const {
    return storage.get("stop_when_track_removed").get<bool>();
  }

  void Global::saveStopWhenTrackRemoved(bool arg) {
    storage.set("stop_when_track_removed", Config::Value(arg));
  }

  int Global::playlistRowHeight() const {
    return storage.get("playlist_row_height").get<int>();
  }

  void Global::savePlaylistRowHeight(int arg) {
    storage.set("playlist_row_height", Config::Value(arg));
  }

  QStringList Global::mprisBlacklist() const {
    auto raw = storage.get("mpris_blacklist");
    QStringList result;
    if (raw.listType() != Config::Value::String) {
      return result;
    }
    auto list = raw.get<QList<Config::Value> >();
    for (const auto &i : std::as_const(list)) {
      result << i.get<QString>();
    }
    return result;
  }

  bool Global::saveMprisBlacklist(const QStringList &arg) {
    QList<Config::Value> list;
    for (const auto &i : std::as_const(arg)) {
      list << Config::Value(i);
    }
    return storage.set("mpris_blacklist", Config::Value(list));
  }

  bool Config::Global::mpdStopPlayerOnClose() const {
    return storage.get("mpd_stop_player_on_close").get<bool>();
  }

  void Global::saveMpdStopPlayerOnClose(bool arg) {
    storage.set("mpd_stop_player_on_close", Config::Value(arg));
  }

  QStringList Global::lyricsProviders() const {
    const QStringList defaults = { "embedded", "sidecar", "lrclib" };
    auto raw = storage.get("lyrics");
    if (raw.type() != Config::Value::Map) {
      return defaults;
    }
    auto map = raw.get<QMap<QString, Config::Value>>();
    if (!map.contains("providers")) {
      return defaults;
    }
    auto list = map.value("providers");
    if (list.type() != Config::Value::List || list.listType() != Config::Value::String) {
      return defaults;
    }
    QStringList result;
    for (const auto &i : list.get<QList<Config::Value>>()) {
      result << i.get<QString>();
    }
    return result.isEmpty() ? defaults : result;
  }

  bool Global::saveLyricsProviders(const QStringList &arg) {
    QList<Config::Value> list;
    for (const auto &i : std::as_const(arg)) {
      list << Config::Value(i);
    }
    QMap<QString, Config::Value> map;
    map.insert("providers", Config::Value(list));
    return storage.set("lyrics", Config::Value(map));
  }

  QString Global::libraryFilterScope() const {
    return storage.get("library_filter_scope").get<QString>();
  }

  void Global::saveLibraryFilterScope(const QString &arg) {
    storage.set("library_filter_scope", Config::Value(arg));
  }

}
