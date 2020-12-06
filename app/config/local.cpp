#include "local.h"

#include <QDebug>

namespace Config {
  Local::Local() : storage("local.yml") {
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

  QList<std::shared_ptr<Playlist::Playlist> > Local::playlists() const {
    auto raw = storage.get("playlists");
    if (raw.listType() != Config::Value::Map) {
      return QList<std::shared_ptr<Playlist::Playlist> >();
    }

    QList<std::shared_ptr<Playlist::Playlist> > result;
    auto list = raw.get<QList<Config::Value> >();
    for (auto i : list) {
      auto map = i.get<QMap<QString, Config::Value>>();
      QString name = map.value("name").get<QString>();
      std::shared_ptr<Playlist::Playlist> playlist(new Playlist::Playlist());
      playlist->rename(name);

      auto order = map.value("playback_order_override").get<int>();
      playlist->setRandom(static_cast<Playlist::Playlist::PlaylistRandom>(order));

      QVector<Track> tracks;
      for (auto t : map.value("tracks").get<QList<Config::Value> >()) {
        tracks << deserializeTrack(t.get<QMap<QString, Config::Value>>());
      }
      playlist->load(tracks);
      result << playlist;
    }
    return result;
  }

  bool Local::savePlaylists(QList<std::shared_ptr<Playlist::Playlist> > &list) {
    QList<Config::Value> plist;
    for (auto i : list) {
      QMap<QString, Config::Value> mp;
      mp.insert("name", Config::Value(i->name()));

      QList<Config::Value> tl;
      for (auto j : i->tracks()) {
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

  QStringList Local::libraryPaths() const {
    auto raw = storage.get("library_paths");
    QStringList result;
    if (raw.listType() != Config::Value::String) {
      return result;
    }
    auto list = raw.get<QList<Config::Value> >();
    for (auto i : list) {
      result << i.get<QString>();
    }
    return result;
  }

  bool Local::saveLibraryPaths(const QStringList &arg) {
    QList<Config::Value> list;
    for (auto i : arg) {
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
            static_cast<quint32>(r["duration"].get<int>()),
            static_cast<quint8>(r["channels"].get<int>()),
            static_cast<quint16>(r["bitrate"].get<int>()),
            static_cast<quint16>(r["samplerate"].get<int>())
          );
      t.setCue(cue);
      return t;
    } else {
     return Track(QUrl(r["url"].get<QString>()), r["path"].get<QString>());
    }
  }
}
