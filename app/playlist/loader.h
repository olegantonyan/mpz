#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include "track.h"

#include <QStringList>
#include <QDir>
#include <QVector>

namespace Playlist {
  class Loader {
  public:
    explicit Loader(const QDir &path);

    static QStringList supportedFileFormats();
    static QStringList supportedPlaylistFileFormats();

    QVector<Track> tracks() const;

  private:
    QDir path;

    bool is_file() const;
    bool is_dir_empty() const;
    bool is_single_file() const;
    bool is_playlist_file() const;
    QStringList files_filter() const;
    bool is_cue(const QString &current_path) const;
  };
}

#endif // PLAYLISTLOADER_H
