#ifndef DROPDIRS_H
#define DROPDIRS_H

#include "playlist/loader.h"

#include <QDir>
#include <QDropEvent>
#include <QFileInfo>
#include <QList>
#include <QMimeData>
#include <QPoint>
#include <QString>
#include <QUrl>

namespace DropUtil {
  inline QList<QDir> droppedDirs(const QMimeData *mime) {
    QList<QDir> dirs;
    if (!mime || !mime->hasUrls()) {
      return dirs;
    }
    for (const auto &url : mime->urls()) {
      const auto path = url.toLocalFile();
      if (path.isEmpty()) {
        continue;
      }
      const QFileInfo info(path);
      if (info.isDir() || (info.isFile() && Playlist::Loader::is_supported_file(path))) {
        dirs << QDir(path);
      }
    }
    return dirs;
  }

  inline QString commonParentDir(const QList<QDir> &dirs) {
    QString common;
    bool first = true;
    for (const auto &dir : dirs) {
      const QString parent = QFileInfo(dir.absolutePath()).absolutePath();
      if (first) {
        common = parent;
        first = false;
      } else if (parent != common) {
        return QString();
      }
    }
    return common;
  }

  inline QPoint dropPosition(QDropEvent *event) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    return event->position().toPoint();
#else
    return event->pos();
#endif
  }
}

#endif // DROPDIRS_H
