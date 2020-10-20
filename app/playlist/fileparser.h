#ifndef FILEPARSER_H
#define FILEPARSER_H

#include "track.h"

#include <QVector>
#include <QDir>
#include <QString>

namespace Playlist {
  class FileParser {
  public:
    explicit FileParser(const QDir &path);

    QVector<Track> tracks_list() const;

  private:
    QDir path;
    QString parseLine(const QString &line, bool &is_stream) const;

    QString current_dir() const;
  };
}

#endif // FILEPARSER_H
