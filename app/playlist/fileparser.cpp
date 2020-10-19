#include "playlist/fileparser.h"
#include "playlist/loader.h"

#include <QDebug>
#include <QFile>
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
    } else if (Loader::is_supported_file(line)) {
      is_stream = false;
      if (QFileInfo(line).isAbsolute()) {
        result = line;
      } else {
        auto dir = QFileInfo(path.absolutePath()).absolutePath();
        auto full_path = QDir(dir).filePath(line);
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
        bool is_stream;
        auto parsed_line = parseLine(line, is_stream);
        if (!parsed_line.isEmpty()) {
          if (is_stream) {
            items << Track(QUrl(parsed_line));
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
