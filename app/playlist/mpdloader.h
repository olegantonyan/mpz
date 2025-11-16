#ifndef PLAYLISTLOADERMPD_H
#define PLAYLISTLOADERMPD_H

#include "track.h"
#include "mpdconnection.h"
#include "playlist/loader.h"

#include <QStringList>
#include <QDir>
#include <QVector>

namespace Playlist {
  class MpdLoader : public Loader {
  public:
    explicit MpdLoader(const QDir &path, MpdConnection &conn);
    explicit MpdLoader(const QString &playlist_name, MpdConnection &conn);

    QVector<Track> tracks() const override;
    bool is_playlist_file() const override;

  private:
    MpdConnection &connection;
    QString playlist_name;

    QVector<Track> loadFromPlaylist(const QString &playlist_name) const;
  };
}

#endif // PLAYLISTLOADERMPD_H
