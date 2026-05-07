#ifndef PLAYLISTLOADER_H
#define PLAYLISTLOADER_H

#include "track.h"

#include <QDir>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Playlist {
  class Loader {
  public:
    explicit Loader(const QDir &path);

    static QStringList supportedFileFormats();
    static QStringList supportedPlaylistFileFormats();
    static bool is_supported_file(const QString &name);

    virtual QVector<Track> tracks() const;
    virtual bool is_playlist_file() const;

  protected:
    QDir path;

  private:

    bool is_dir_empty() const;
    bool is_single_file() const;
    QStringList files_filter() const;
    bool is_cue(const QString &current_path) const;
    void remove_tracks_added_from_cue(const QSet<QString> &cue_audio_paths, QVector<Track> &tracks) const;
  };
}

#endif // PLAYLISTLOADER_H
