/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>
   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CUEPARSER_H
#define CUEPARSER_H

#include "track.h"

#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QUrl>
#include <QVector>

// This parser will try to detect the real encoding of a .cue file but there's
// a great chance it will fail so it's probably best to assume that the parser
// is UTF compatible only.
namespace Playlist {
  class CueParser {
  public:
    explicit CueParser(const QString &path);

    QVector<Track> tracks_list() const;

  private:
    QString path;

    QStringList split_cue_line(const QString& line) const;
    qint32 begin_by_index(const QString& index) const;

    // A single TRACK entry in .cue file.
    class CueEntry {
    public:
      QString file;

      QString index;

      QString title;
      QString artist;
      QString album_artist;
      QString album;

      QString composer;
      QString album_composer;

      QString genre;
      QString date;
      QString disc;

      CueEntry(QString& file, QString& index, QString& title, QString& artist,
               QString& album_artist, QString& album, QString& composer,
               QString& album_composer, QString& genre, QString& date, QString& disc) {
        this->file = file;
        this->index = index;
        this->title = title;
        this->artist = artist;
        this->album_artist = album_artist;
        this->album = album;
        this->composer = composer;
        this->album_composer = album_composer;
        this->genre = genre;
        this->date = date;
        this->disc = disc;
      }
    };
  };
}



#endif  // CUEPARSER_H
