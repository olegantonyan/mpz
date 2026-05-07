#ifndef CUEPARSER_H
#define CUEPARSER_H

#include "track.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace Playlist {
  // Parses .cue sheets into a list of virtual Tracks.
  //
  // Robust against:
  //   - quoted/unquoted/whitespace-separated values
  //   - ';' comment lines
  //   - UTF-8/UTF-16 BOMs
  //   - Windows-style backslashes in FILE paths on Unix
  //   - case-different filenames on case-sensitive filesystems
  //   - non-AUDIO tracks (BINARY/MODE1/etc. are skipped)
  //   - missing INDEX 01 (falls back to INDEX 00)
  //   - multi-FILE cues, per-track cues, and cues with no tracks
  class CueParser {
  public:
    explicit CueParser(const QString& path);

    QVector<Track> tracks_list() const;

  private:
    struct Entry {
      QString file;        // resolved absolute path of audio file
      int track_no = 0;    // value from `TRACK NN`
      qint64 index00_ms = -1;
      qint64 index01_ms = -1;
      QString title;
      QString artist;
      QString composer;
    };

    QString path_;

    static QStringList tokenize(const QString& line);
    static qint64 parse_index_position(const QString& mss_ff);
    static QString resolve_audio_file(const QString& cue_dir, const QString& referenced);
    static quint16 parse_year(const QString& s);
  };
}

#endif // CUEPARSER_H
