#ifndef LOCAL_H
#define LOCAL_H

#include "storage.h"
#include "playlist/playlist.h"

#include <QByteArray>

namespace Config {
  class Local {
  public:
    explicit Local();

    bool sync();

    bool saveWindowGeometry(const QByteArray &v);
    QByteArray windowGeomentry() const;

    bool saveWindowState(const QByteArray &v);
    QByteArray windowState() const;

    bool saveSplitterSizes(const QList<int> &list);
    QList<int> splitterSizes() const;

    QList<std::shared_ptr<Playlist::Playlist>> playlists() const;
    bool savePlaylists(QList<std::shared_ptr<Playlist::Playlist>> &list);

    int currentPlaylist() const;
    bool saveCurrentPlaylist(int idx);

    QStringList libraryPaths() const;
    bool saveLibraryPaths(const QStringList &arg);

    int currentLibraryPath() const;
    bool saveCurrentLibraryPath(int arg);

    int libraryViewScrollPosition() const;
    bool saveLibraryViewScrollPosition(int val);

    int playlistViewScrollPosition() const;
    bool savePlaylistViewScrollPosition(int val);

    int volume() const;
    bool saveVolume(int arg);

    int totalPlaybackTime() const;
    bool saveTotalPlaybackTime(int arg);

  private:
    Config::Storage storage;

    Config::Value serializeTrack(const Track &t) const;
    Track deserializeTrack(const Config::Value &v) const;
  };
}

#endif // LOCAL_H
