#ifndef LOCAL_H
#define LOCAL_H

#include "storage.h"
#include "playlist.h"

#include <QByteArray>

namespace Config {
  class Local {
  public:
    Local();

    bool sync();

    bool saveWindowGeometry(const QByteArray &v);
    QByteArray windowGeomentry() const;

    bool saveWindowState(const QByteArray &v);
    QByteArray windowState() const;

    bool saveSplitterSizes(const QList<int> &list);
    QList<int> splitterSizes() const;

    QList<std::shared_ptr<Playlist>> playlists() const;
    bool savePlaylists(QList<std::shared_ptr<Playlist>> &list);

    int currentPlaylist() const;
    bool saveCurrentPlaylist(int idx);

    QStringList libraryPaths() const;

    int libraryViewScrollPosition() const;
    bool saveLibraryViewScrollPosition(int val);

  private:
    Config::Storage storage;
  };
}

#endif // LOCAL_H
