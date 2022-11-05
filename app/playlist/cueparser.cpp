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

#include "cueparser.h"

#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QStringBuilder>
#include <QTextStream>
#include <QtDebug>
#include <QFileInfo>
#include <QtGlobal>
#include <QDebug>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #include <QStringConverter>
  #include <QRegularExpression>
  #include <QRegularExpressionMatch>
#else
  #include <QTextCodec>
  #include <QRegExp>
#endif

namespace Playlist {
  static const char* kFileLineRegExp = "(\\S+)\\s+(?:\"([^\"]+)\"|(\\S+))\\s*(?:\"([^\"]+)\"|(\\S+))?";
  static const char* kIndexRegExp = "(\\d{2,3}):(\\d{2}):(\\d{2})";

  static const char* kPerformer = "performer";
  static const char* kTitle = "title";
  static const char* kSongWriter = "songwriter";
  static const char* kFile = "file";
  static const char* kTrack = "track";
  static const char* kIndex = "index";
  static const char* kAudioTrackType = "audio";
  static const char* kRem = "rem";
  static const char* kGenre = "genre";
  static const char* kDate = "date";
  static const char* kDisc = "discnumber";

  CueParser::CueParser(const QString &p) : path(p) {
  }

  QVector<Track> CueParser::tracks_list() const {

    QFile device(path);
    if (!device.open(QIODevice::ReadOnly)) {
      qWarning() << "error opening file for reading" << path << ":" << device.errorString();
      return QVector<Track>();
    }

    QTextStream text_stream(&device);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    text_stream.setEncoding(QStringConverter::encodingForData(device.peek(1024)).value_or(QStringConverter::Utf8));
#else
    text_stream.setCodec(QTextCodec::codecForUtfText(device.peek(1024), QTextCodec::codecForName("UTF-8")));
#endif

    QString dir_path = QFileInfo(path).absoluteDir().absolutePath();
    // read the first line already
    QString line = text_stream.readLine();

    QList<CueEntry> entries;

    QString album_artist;
    QString album;
    QString album_composer;
    QString file;
    QString file_type;
    QString genre;
    QString date;
    QString disc;

    // -- whole file
    while (!text_stream.atEnd()) {

      // -- FILE section
      do {
        QStringList splitted = split_cue_line(line);

        // uninteresting or incorrect line
        if (splitted.size() < 2) {
          continue;
        }

        QString line_name = splitted[0].toLower();
        QString line_value = splitted[1];

        // PERFORMER
        if (line_name == kPerformer) {
          album_artist = line_value;

          // TITLE
        } else if (line_name == kTitle) {
          album = line_value;

          // SONGWRITER
        } else if (line_name == kSongWriter) {
          album_composer = line_value;

          // FILE
        } else if (line_name == kFile) {
          file = QDir::isAbsolutePath(line_value) ? line_value : QDir(dir_path).absoluteFilePath(line_value);

          if (splitted.size() > 2) {
            file_type = splitted[2];
          }

          // REM
        } else if (line_name == kRem) {
          if (splitted.size() < 3) {
            break;
          }

          // REM GENRE
          if (line_value.toLower() == kGenre) {
            genre = splitted[2];

            // REM DATE
          } else if (line_value.toLower() == kDate) {
            date = splitted[2];

            // REM DISC
          } else if (line_value.toLower() == kDisc) {
            disc = splitted[2];
          }

          // end of the header -> go into the track mode
        } else if (line_name == kTrack) {
          break;
        }

        // just ignore the rest of possible field types for now...
      } while (!(line = text_stream.readLine()).isNull());

      if (line.isNull()) {
        qWarning() << "the .cue file from " << dir_path << " defines no tracks!";
        //return ret;
      }

      // if this is a data file, all of it's tracks will be ignored
      bool valid_file = file_type.compare("BINARY", Qt::CaseInsensitive) && file_type.compare("MOTOROLA", Qt::CaseInsensitive);

      QString track_type;
      QString index;
      QString artist;
      QString composer;
      QString title;

      // TRACK section
      do {
        QStringList splitted = split_cue_line(line);

        // uninteresting or incorrect line
        if (splitted.size() < 2) {
          continue;
        }

        QString line_name = splitted[0].toLower();
        QString line_value = splitted[1];
        QString line_additional = splitted.size() > 2 ? splitted[2].toLower() : "";

        if (line_name == kTrack) {
          // the beginning of another track's definition - we're saving the
          // current one
          // for later (if it's valid of course)
          // please note that the same code is repeated just after this 'do-while'
          // loop
          if (valid_file && !index.isEmpty() && (track_type.isEmpty() || track_type == kAudioTrackType)) {
            entries.append(CueEntry(file, index, title, artist, album_artist, album, composer, album_composer, genre, date, disc));
          }

          // clear the state
          track_type = index = artist = title = "";

          if (!line_additional.isEmpty()) {
            track_type = line_additional;
          }

        } else if (line_name == kIndex) {
          // we need the index's position field
          if (!line_additional.isEmpty()) {
            // if there's none "01" index, we'll just take the first one
            // also, we'll take the "01" index even if it's the last one
            if (line_value == "01" || index.isEmpty()) {
              index = line_additional;
            }
          }

        } else if (line_name == kPerformer) {
          artist = line_value;

        } else if (line_name == kTitle) {
          title = line_value;

        } else if (line_name == kSongWriter) {
          composer = line_value;

          // end of track's for the current file -> parse next one
        } else if (line_name == kFile) {
          break;
        }

        // just ignore the rest of possible field types for now...
      } while (!(line = text_stream.readLine()).isNull());

      // we didn't add the last song yet...
      if (valid_file && !index.isEmpty() && (track_type.isEmpty() || track_type == kAudioTrackType)) {
        entries.append(CueEntry(file, index, title, artist, album_artist, album, composer, album_composer, genre, date, disc));
      }
    }

    QVector<Track> tracks;

    // finalize parsing songs
    for (int i = 0; i < entries.length(); i++) {
      CueEntry entry = entries.at(i);

      //qDebug() << entry.index << begin_by_index(entry.index) << entry.title << entry.artist << entry.album;

      bool fuck;
      Track track(entry.file,
                  begin_by_index(entry.index),
                  entry.artist.isEmpty() ? entry.album_artist : entry.artist,
                  entry.album,
                  entry.title,
                  i + 1,
                  entry.date.toUInt(&fuck),
                  0,
                  0,
                  0,
                  0);
      track.fillAudioProperties();
      track.setCue();
      if (i < entries.length() - 1) {
        CueEntry next_enrty = entries.at(qMin(i + 1, entries.length() - 1));
        qint32 duration = begin_by_index(next_enrty.index) - begin_by_index(entry.index);
        if (duration < 0) {
          quint32 duration = track.duration() - begin_by_index(entry.index); // last track for this file - got duration from audio properties
          track.setDuration(duration);
        } else {
          track.setDuration(duration);
        }
      } else {
        quint32 duration = track.duration() - begin_by_index(entry.index); // got duration from audio properties
        track.setDuration(duration);
      }

      //qDebug() << track.track_number() << track.title() << track.artist();

      tracks << track;
    }

    return tracks;
  }

  // This and the kFileLineRegExp do most of the "dirty" work, namely: splitting
  // the raw .cue
  // line into logical parts and getting rid of all the unnecessary whitespaces
  // and quoting.
  QStringList CueParser::split_cue_line(const QString& line) const {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QRegularExpression line_regexp(kFileLineRegExp);
    QRegularExpressionMatch re_match = line_regexp.match(line.trimmed());
    if (!re_match.hasMatch()) {
      return QStringList();
    }

    // Let's remove the empty entries while we're at it
    return re_match.capturedTexts().filter(QRegularExpression(".+")).mid(1, -1).replaceInStrings(QRegularExpression("^\"\"$"), "");
#else
    QRegExp line_regexp(kFileLineRegExp);
    if (!line_regexp.exactMatch(line.trimmed())) {
      return QStringList();
    }

    // let's remove the empty entries while we're at it
    return line_regexp.capturedTexts().filter(QRegExp(".+")).mid(1, -1);
#endif
  }

  qint32 CueParser::begin_by_index(const QString& index) const {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QRegularExpression index_regexp(kIndexRegExp);
    QRegularExpressionMatch re_match = index_regexp.match(index);
    if (!re_match.hasMatch()) {
      return -1;
    }

    QStringList splitted = re_match.capturedTexts().mid(1, -1);
#else
    QRegExp index_regexp(kIndexRegExp);
    if (!index_regexp.exactMatch(index)) {
      return -1;
    }

    QStringList splitted = index_regexp.capturedTexts().mid(1, -1);
#endif

    qlonglong frames = splitted.at(0).toLongLong() * 60 * 75 +
                       splitted.at(1).toLongLong() * 75 +
                       splitted.at(2).toLongLong();

    return frames / 75; // seconds
  }
}
