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
    return QList<std::shared_ptr<Playlist> >();
  }

  bool Local::savePlaylists(QList<std::shared_ptr<Playlist> > &list) {
    QList<Config::Value> plist;
    for (auto i : list) {
      QMap<QString, Config::Value> mp;
      mp.insert("name", Config::Value(i->name()));
      mp.insert("path", Config::Value(i->path().absolutePath()));
      plist << Config::Value(mp);
    }
    return storage.set("playlists", Config::Value(plist));
  }
}
