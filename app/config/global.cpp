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

  QVector<Radio::Station> Global::radioStations() const {
    QVector<Radio::Station> result;
    auto raw = storage.get("radio_stations");
    if (raw.listType() != Config::Value::Map) {
      return result;
    }
    const auto list = raw.get<QList<Config::Value>>();
    for (const auto &i : list) {
      auto m = i.get<QMap<QString, Config::Value>>();
      Radio::Station s;
      s.id = m.value("id").get<QString>();
      s.name = m.value("name").get<QString>();
      s.group = m.value("group").get<QString>();
      s.url = m.value("url").get<QString>();
      s.codec = m.value("codec").get<QString>();
      s.bitrate = static_cast<quint16>(m.value("bitrate").get<int>());
      s.homepage = m.value("homepage").get<QString>();
      result << s;
    }
    return result;
  }

  bool Global::saveRadioStations(const QVector<Radio::Station> &arg) {
    QList<Config::Value> list;
    for (const auto &s : arg) {
      QMap<QString, Config::Value> m;
      m.insert("id", Config::Value(s.id));
      m.insert("name", Config::Value(s.name));
      m.insert("group", Config::Value(s.group));
      m.insert("url", Config::Value(s.url));
      m.insert("codec", Config::Value(s.codec));
      m.insert("bitrate", Config::Value(static_cast<int>(s.bitrate)));
      m.insert("homepage", Config::Value(s.homepage));
      list << Config::Value(m);
    }
    // An empty list carries no element type, so tag it to keep the Map type.
    Config::Value value(list);
    value.setListType(Config::Value::Map);
    return storage.set("radio_stations", value);
  }

  bool Global::disableGapless() const {
    return storage.get("disable_gapless").get<bool>();
  }

  void Global::saveDisableGapless(bool arg) {
    storage.set("disable_gapless", Config::Value(arg));
  }

  int Global::gaplessCacheSizeMb() const {
    return storage.get("gapless_cache_size_mb").get<int>();
  }

  void Global::saveGaplessCacheSizeMb(int arg) {
    storage.set("gapless_cache_size_mb", Config::Value(arg));
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
    return providersUnder("lyrics");
  }

  bool Global::saveLyricsProviders(const QStringList &arg) {
    return saveProvidersUnder("lyrics", arg);
  }

  QStringList Global::coverProviders() const {
    return providersUnder("covers");
  }

  bool Global::saveCoverProviders(const QStringList &arg) {
    return saveProvidersUnder("covers", arg);
  }

  // Online providers only, and there are no defaults: anything missing or
  // malformed is simply "no online providers". Built-in sources are not
  // configurable and never appear here. Legacy entries naming them are returned
  // as-is and dropped downstream by ProviderChain::filterKnown.
  QStringList Global::providersUnder(const QString &key) const {
    auto raw = storage.get(key);
    if (raw.type() != Config::Value::Map) {
      return {};
    }
    auto list = raw.get<QMap<QString, Config::Value>>().value("providers");
    if (list.type() != Config::Value::List) {
      return {};
    }
    QStringList result;
    for (const auto &i : list.get<QList<Config::Value>>()) {
      if (i.type() == Config::Value::String) {
        result << i.get<QString>();
      }
    }
    return result;
  }

  bool Global::saveProvidersUnder(const QString &key, const QStringList &arg) {
    QList<Config::Value> list;
    for (const auto &i : std::as_const(arg)) {
      list << Config::Value(i);
    }
    QMap<QString, Config::Value> map;
    map.insert("providers", Config::Value(list));
    return storage.set(key, Config::Value(map));
  }

  QString Global::libraryFilterScope() const {
    return storage.get("library_filter_scope").get<QString>();
  }

  void Global::saveLibraryFilterScope(const QString &arg) {
    storage.set("library_filter_scope", Config::Value(arg));
  }

}
