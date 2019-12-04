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

  QList<std::shared_ptr<Playlist> > Local::playlists() const {
    auto raw = storage.get("playlists");
    if (raw.listType() != Config::Value::Map) {
      return QList<std::shared_ptr<Playlist> >();
    }

    QList<std::shared_ptr<Playlist> > result;
    auto list = raw.get<QList<Config::Value> >();
    for (auto i : list) {
      auto map = i.get<QMap<QString, Config::Value>>();
      QString name = map.value("name").get<QString>();
      std::shared_ptr<Playlist> playlist(new Playlist());
      playlist->rename(name);
      QVector<Track> tracks;
      for (auto t : map.value("tracks").get<QList<Config::Value> >()) {
        tracks << Track(t.get<QString>());
      }
      playlist->load(tracks);
      result << playlist;
    }
    return result;
  }

  bool Local::savePlaylists(QList<std::shared_ptr<Playlist> > &list) {
    QList<Config::Value> plist;
    for (auto i : list) {
      QMap<QString, Config::Value> mp;
      mp.insert("name", Config::Value(i->name()));

      QList<Config::Value> tl;
      for (auto j : i->tracks()) {
        tl << Config::Value(j.path());
      }
      mp.insert("tracks", tl);

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

  int Local::libraryViewScrollPosition() const {
    return storage.get("library_view_scroll").get<int>();
  }

  bool Local::saveLibraryViewScrollPosition(int val) {
    return storage.set("library_view_scroll", Config::Value(val));
  }
}
