#include "local.h"

#include <QDebug>
#include <QApplication>

namespace Config {
  Local::Local() : SingleInstanceGuard("Config::Local"), storage("local.yml") {
    if (storage.appVersion().isNull() || storage.appVersion() < QVersionNumber(1, 1, 0)) {
      durationSeconds = true;
    } else {
      durationSeconds = false;
    }
  }

  bool Local::sync() {
    return storage.save();
  }

  bool Local::saveWindowGeometry(const QByteArray &v) {
    return storage.set("window_geometry", v);
  }

  QByteArray Local::windowGeomentry() const {
    return storage.getByteArray("window_geometry");
  }

  bool Local::saveWindowState(const QByteArray &v) {
    return storage.set("window_state", v);
  }

  QByteArray Local::windowState() const {
    return storage.getByteArray("window_state");
  }

  bool Local::saveSplitterSizes(const QList<int> &list) {
    return storage.set("splitter_sizes", list);
  }

  QList<int> Local::splitterSizes() const {
    return storage.getIntList("splitter_sizes");
  }

  bool Local::toolbarMovable() const {
    return storage.get("toolbar_movable").get<bool>();
  }

  bool Local::saveToolbarMovable(bool v) {
    return storage.set("toolbar_movable", Config::Value(v));
  }

  QList<std::shared_ptr<Playlist::Playlist> > Local::playlists() const {
    auto raw = storage.get("playlists");
    if (raw.listType() != Config::Value::Map) {
      return QList<std::shared_ptr<Playlist::Playlist> >();
    }

    QList<std::shared_ptr<Playlist::Playlist> > result;
    auto list = raw.get<QList<Config::Value> >();
    for (const auto &i : std::as_const(list)) {
      auto map = i.get<QMap<QString, Config::Value>>();
      QString name = map.value("name").get<QString>();
      std::shared_ptr<Playlist::Playlist> playlist(new Playlist::Playlist());
      playlist->rename(name);

      auto order = map.value("playback_order_override").get<int>();
      playlist->setRandom(static_cast<Playlist::Playlist::PlaylistRandom>(order));

      QVector<Track> tracks;
      for (const auto &t : map.value("tracks").get<QList<Config::Value> >()) {
        tracks << deserializeTrack(t.get<QMap<QString, Config::Value>>());
      }
      playlist->load(tracks);
      result << playlist;
    }
    return result;
  }

  bool Local::savePlaylists(QList<std::shared_ptr<Playlist::Playlist> > &list) {
    QList<Config::Value> plist;
    for (const auto &i : std::as_const(list)) {
      QMap<QString, Config::Value> mp;
      mp.insert("name", Config::Value(i->name()));

      QList<Config::Value> tl;
      for (const auto &j : i->tracks()) {
        tl << serializeTrack(j);
      }
      mp.insert("tracks", tl);
      mp.insert("playback_order_override", static_cast<int>(i->random()));

      plist << Config::Value(mp);
    }
    return storage.set("playlists", Config::Value(plist));
  }

  int Local::currentPlaylist() const {
    return storage.get("current_playlist").get<int>();
  }

  bool Local::saveCurrentPlaylist(int idx) {
    return storage.set("current_playlist", Config::Value(idx));
  }

  QMap<QString, QString> Local::currentMpdPlaylist() const {
    auto mv = storage.get("current_mpd_playlist").get<QMap<QString, Config::Value>>();
    QMap<QString, QString> result;

    for (auto it = mv.constBegin(); it != mv.constEnd(); ++it) {
      result[it.key()] = it.value().get<QString>();
    }

    return result;
  }

  bool Local::saveCurrentMpdPlaylist(const QMap<QString, QString> &name_for_library_url) {
    QMap<QString, Config::Value> mv;

    for (auto it = name_for_library_url.constBegin(); it != name_for_library_url.constEnd(); ++it) {
      mv[it.key()] = Config::Value(it.value());
    }

    return storage.set("current_mpd_playlist", Config::Value(mv));
  }

  QMap<QString, QList<QString>> Local::mpdPlaylistsOrder() const {
    auto mv = storage.get("mpd_playlists_order").get<QMap<QString, Config::Value>>();
    QMap<QString, QList<QString>> result;

    for (auto it = mv.constBegin(); it != mv.constEnd(); ++it) {
      QList<QString> list;
      auto raw = it.value().get<QList<Config::Value>>();
      for (const auto &li : std::as_const(raw)) {
        list << li.get<QString>();
      }
      result[it.key()] = list;
    }

    return result;
  }

  bool Local::saveMpdPlaylistsOrder(const QMap<QString, QList<QString>> &playlists_for_library_url) {
    QMap<QString, Config::Value> mv;

    for (auto it = playlists_for_library_url.constBegin(); it != playlists_for_library_url.constEnd(); ++it) {
      QList<Config::Value> val;
      for (const auto &it : it.value()) {
        val << Config::Value(it);
      }
      mv[it.key()] = Config::Value(val);
    }
    return storage.set("mpd_playlists_order", Config::Value(mv));
  }

  QStringList Local::libraryPaths() const {
    auto raw = storage.get("library_paths");
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

  bool Local::saveLibraryPaths(const QStringList &arg) {
    QList<Config::Value> list;
    for (const auto &i : std::as_const(arg)) {
      list << Config::Value(i);
    }
    return storage.set("library_paths", Config::Value(list));
  }

  int Local::currentLibraryPath() const {
    return storage.get("current_library").get<int>();
  }

  bool Local::saveCurrentLibraryPath(int arg) {
    return storage.set("current_library", Config::Value(arg));
  }

  int Local::libraryViewScrollPosition() const {
    return storage.get("library_view_scroll").get<int>();
  }

  bool Local::saveLibraryViewScrollPosition(int val) {
    return storage.set("library_view_scroll", Config::Value(val));
  }

  int Local::playlistViewScrollPosition() const {
    return storage.get("playlist_view_scroll").get<int>();
  }

  bool Local::savePlaylistViewScrollPosition(int val) {
    return storage.set("playlist_view_scroll", Config::Value(val));
  }

  int Local::volume() const {
    return storage.get("playback_volume").get<int>();
  }

  bool Local::saveVolume(int arg) {
    return storage.set("playback_volume", Config::Value(arg));
  }

  int Local::totalPlaybackTime() const {
    return storage.get("total_playback_time").get<int>();
  }

  bool Local::saveTotalPlaybackTime(int arg) {
    return storage.set("total_playback_time", Config::Value(arg));
  }

  QByteArray Local::outputDeviceId() const {
    return storage.getByteArray("output_device_id");
  }

  bool Local::saveOutputDeviceId(const QByteArray &arg) {
    return storage.set("output_device_id", arg);
  }

  bool Local::eqEnabled() const {
    return storage.get("eq_enabled").get<bool>();
  }

  bool Local::saveEqEnabled(bool arg) {
    return storage.set("eq_enabled", Config::Value(arg));
  }

  QList<Eq::EqProfile> Local::eqProfiles() const {
    QList<Eq::EqProfile> result;
    auto raw = storage.get("eq_profiles");
    if (raw.listType() != Config::Value::Map) {
      return result;
    }
    for (const auto &i : raw.get<QList<Config::Value>>()) {
      auto map = i.get<QMap<QString, Config::Value>>();
      Eq::EqProfile p;
      p.name = map.value("name").get<QString>();
      p.enabled = map.value("enabled").get<bool>();
      p.preamp_db = map.value("preamp").get<QString>().toDouble();
      p.auto_preamp = map.value("auto_preamp").get<bool>();
      for (const auto &bv : map.value("bands").get<QList<Config::Value>>()) {
        auto bm = bv.get<QMap<QString, Config::Value>>();
        Eq::Band b;
        b.type = Eq::bandTypeFromString(bm.value("type").get<QString>());
        b.freq_hz = bm.value("freq").get<QString>().toDouble();
        b.gain_db = bm.value("gain").get<QString>().toDouble();
        b.q = bm.value("q").get<QString>().toDouble();
        b.enabled = bm.value("enabled").get<bool>();
        p.bands.push_back(b);
      }
      result << p;
    }
    return result;
  }

  bool Local::saveEqProfiles(const QList<Eq::EqProfile> &arg) {
    QList<Config::Value> list;
    for (const auto &p : arg) {
      QMap<QString, Config::Value> pm;
      pm.insert("name", Config::Value(p.name));
      pm.insert("enabled", Config::Value(p.enabled));
      pm.insert("preamp", Config::Value(QString::number(p.preamp_db, 'g', 10)));
      pm.insert("auto_preamp", Config::Value(p.auto_preamp));
      QList<Config::Value> bands;
      for (const auto &b : p.bands) {
        QMap<QString, Config::Value> bm;
        bm.insert("type", Config::Value(Eq::bandTypeToString(b.type)));
        bm.insert("freq", Config::Value(QString::number(b.freq_hz, 'g', 10)));
        bm.insert("gain", Config::Value(QString::number(b.gain_db, 'g', 10)));
        bm.insert("q", Config::Value(QString::number(b.q, 'g', 10)));
        bm.insert("enabled", Config::Value(b.enabled));
        bands << Config::Value(bm);
      }
      Config::Value bands_value(bands);
      bands_value.setListType(Config::Value::Map);
      pm.insert("bands", bands_value);
      list << Config::Value(pm);
    }
    Config::Value value(list);
    value.setListType(Config::Value::Map);
    return storage.set("eq_profiles", value);
  }

  QString Local::eqActiveProfile() const {
    return storage.get("eq_active_profile").get<QString>();
  }

  bool Local::saveEqActiveProfile(const QString &arg) {
    return storage.set("eq_active_profile", Config::Value(arg));
  }

  QMap<QString, QString> Local::eqDeviceProfiles() const {
    QMap<QString, QString> result;
    auto mv = storage.get("eq_device_profiles").get<QMap<QString, Config::Value>>();
    for (auto it = mv.constBegin(); it != mv.constEnd(); ++it) {
      result[it.key()] = it.value().get<QString>();
    }
    return result;
  }

  bool Local::saveEqDeviceProfiles(const QMap<QString, QString> &arg) {
    QMap<QString, Config::Value> mv;
    for (auto it = arg.constBegin(); it != arg.constEnd(); ++it) {
      mv[it.key()] = Config::Value(it.value());
    }
    return storage.set("eq_device_profiles", Config::Value(mv));
  }

  QString Local::crashReportConsent() const {
    return storage.get("crash_report_consent").get<QString>();
  }

  void Local::saveCrashReportConsent(const QString &arg) {
    storage.set("crash_report_consent", Config::Value(arg));
  }

  QString Local::lastReportedCrash() const {
    return storage.get("last_reported_crash").get<QString>();
  }

  void Local::saveLastReportedCrash(const QString &arg) {
    storage.set("last_reported_crash", Config::Value(arg));
  }

  Value Local::serializeTrack(const Track &t) const {
    QMap<QString, Config::Value> r;
    r["path"] = t.path();
    r["begin"] = static_cast<int>(t.begin());
    r["cue"] = t.isCue();
    r["artist"] = t.artist();
    r["album"] = t.album();
    r["title"] = t.title();
    r["track_number"] = static_cast<int>(t.track_number());
    r["year"] = static_cast<int>(t.year());
    r["duration"] = static_cast<int>(t.duration());
    r["channels"] = static_cast<int>(t.channels());
    r["bitrate"] = static_cast<int>(t.bitrate());
    r["samplerate"] = static_cast<int>(t.sample_rate());
    if (t.isStream()) {
      r["url"] = t.url().toString();
    }
    return r;
  }

  Track Local::deserializeTrack(const Value &v) const {
    auto r = v.get<QMap<QString, Value>>();
    if (r["url"].get<QString>().isEmpty()) {
      bool cue = r["cue"].get<bool>();
      Track t(
            r["path"].get<QString>(),
            static_cast<quint32>(r["begin"].get<int>()),
            r["artist"].get<QString>(),
            r["album"].get<QString>(),
            r["title"].get<QString>(),
            static_cast<quint16>(r["track_number"].get<int>()),
            static_cast<quint16>(r["year"].get<int>()),
            trackDuration(static_cast<quint64>(r["duration"].get<int>())),
            static_cast<quint8>(r["channels"].get<int>()),
            static_cast<quint16>(r["bitrate"].get<int>()),
            static_cast<quint32>(r["samplerate"].get<int>())
          );
      t.setCue(cue);
      return t;
    } else {
     return Track(QUrl(r["url"].get<QString>()), r["path"].get<QString>(), r["title"].get<QString>());
    }
  }

  quint64 Local::trackDuration(int value) const {
    if (durationSeconds) {
      return value * 1000;
    } else {
      return value;
    }
  }
}
