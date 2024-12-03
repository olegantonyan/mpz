#include "coverart/covers.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QBuffer>

namespace CoverArt {
  Covers &Covers::instance() {
    static Covers *self = nullptr;
    if (nullptr == self) {
      self = new Covers();
    }
    return *self;
  }

  Covers::Covers() {
  }

  QString Covers::get(const QString &filepath) {
    if (filepath.isEmpty()) {
      return QString();
    }

    auto key = keyByFilepath(filepath);
    if (cache.contains(key)) {
      return cache.value(key);
    }

    auto found = findCoverLocally(key);
    if (found.isEmpty()) {
      auto img = embedded_covers.get(filepath);
      if (!img.isEmpty()) {
        return img;
      }
    } else {
      cache.insert(key, found);
    }
    return found;
  }

  QString Covers::keyByFilepath(const QString &filepath) const {
    return QFileInfo(filepath).absoluteDir().absolutePath();
  }

  QString Covers::findCoverLocally(const QString &dir) {
    const auto nmask = QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.gif" << "*.tiff" << "*.bmp";

    QString result;
    QDirIterator it(dir, nmask, QDir::Files, QDirIterator::NoIteratorFlags);
    int index = 0;
    while (it.hasNext()) {
      auto current_file = it.next();
      if (index == 0) {
        result = current_file; // fallback to first image file found in the dir
      }

      if (current_file.contains("cover", Qt::CaseInsensitive)) {
        result = current_file;
        break;
      }
    }
    return result;
  }
}
