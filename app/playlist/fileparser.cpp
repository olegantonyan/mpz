#include "fileparser.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>

namespace Playlist {
  FileParser::FileParser(const QDir &p) : path(p) {
  }

  QString FileParser::parseLine(const QString &line) const {
    QString result;

    if (line.startsWith("http", Qt::CaseInsensitive)) { // m3u
      result = line;
    } else if (line.startsWith("File", Qt::CaseInsensitive)) { // pls
      result = line.mid(6);
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
        auto parsed_line = parseLine(line);
        if (!parsed_line.isEmpty()) {
          items << Track(QUrl(parsed_line));
        }
      }
    }
    file.close();

    return items;
  }

}
