#include "playlist/fileparser.h"
#include "playlist/loader.h"

#include <QDebug>
#include <QFile>
#include <QUrl>
#include <QTextStream>
#include <QFileInfo>

namespace Playlist {
  FileParser::FileParser(const QDir &p) : path(p) {
  }

  QString FileParser::parseLine(const QString &line, bool &is_stream) const {
    QString result;

    if (line.startsWith("http", Qt::CaseInsensitive)) { // m3u stream
      result = line;
      is_stream = true;
    } else if (line.startsWith("File", Qt::CaseInsensitive)) { // pls stream
      result = line.mid(6);
      is_stream = true;
    } else if (Loader::is_supported_file(line)) { // probably local m3u
      is_stream = false;
      QFileInfo fi(line);
      if (fi.isAbsolute() && fi.exists()) {
        result = line;
      } else if (fi.isRelative()) {
        auto full_path = QDir(current_dir()).filePath(line);
        if (QFileInfo(full_path).exists()) {
          result = full_path;
        }
      }
    }

    if (result.contains(".pls", Qt::CaseInsensitive)) { // bullshit format - link to a pls file instead of stream itself
      qDebug() << "TODO: bullshit pls format" << result;
      // TODO
      // also maybe better to move this into player itself and fetch every time before playing
    }

    return result;
  }

  QString FileParser::current_dir() const {
    return QFileInfo(path.absolutePath()).absolutePath();
  }

  QVector<Track> FileParser::tracks_list() const {
    QFile file(path.absolutePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "error opening file for reading" << path.absolutePath() << ":" << file.errorString();
      return QVector<Track>();
    }

    QVector<Track> items;
    QTextStream in(&file);
    while (!in.atEnd()) {
      auto line = in.readLine().trimmed();
      if (!line.startsWith("#") && !line.isEmpty()) {
        bool is_stream = false;
        auto parsed_line = parseLine(line, is_stream);
        if (!parsed_line.isEmpty()) {
          if (is_stream) {
            items << Track(QUrl(parsed_line), path.absolutePath());
          } else {
            items << Track(parsed_line);
          }
        }
      }
    }
    file.close();

    return items;
  }

}
