#include "diskcache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace DiskCache {
  namespace {
    const qint64 MISS_TTL_SECONDS = 30LL * 24 * 60 * 60;
  }

  Store::Store(const QString &subdir) {
    cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                QDir::separator() + subdir + QDir::separator();
    if (!QDir(cache_dir).exists()) {
      QDir().mkpath(cache_dir);
    }
  }

  QString Store::hash(const QString &first, const QString &second) {
    const QString a = first.trimmed().toCaseFolded();
    const QString b = second.trimmed().toCaseFolded();
    if (a.isEmpty() || b.isEmpty()) {
      return QString();
    }
    const QByteArray raw = (a + QChar(0x1F) + b).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex());
  }

  QString Store::find(const QString &key, const QStringList &exts) const {
    if (key.isEmpty()) {
      return QString();
    }
    for (const auto &ext : exts) {
      const QString path = cache_dir + key + "." + ext;
      if (QFile::exists(path)) {
        return path;
      }
    }
    return QString();
  }

  QString Store::write(const QString &key, const QString &ext, const QByteArray &data) {
    if (key.isEmpty() || data.isEmpty()) {
      return QString();
    }
    const QString path = cache_dir + key + "." + ext;
    // QSaveFile is atomic and replaces an existing file on Windows, which a
    // plain QFile::rename does not.
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
      return QString();
    }
    f.write(data);
    if (!f.commit()) {
      return QString();
    }
    return path;
  }

  QString Store::missPath(const QString &key, const QStringList &tags) const {
    if (key.isEmpty()) {
      return QString();
    }
    QStringList sorted = tags;
    sorted.sort();
    const QByteArray tag = QCryptographicHash::hash(sorted.join(QChar(0x1F)).toUtf8(),
                                                    QCryptographicHash::Sha1).toHex().left(8);
    return cache_dir + key + "-" + QString::fromLatin1(tag) + ".miss";
  }

  bool Store::isKnownMiss(const QString &key, const QStringList &tags) const {
    const QString path = missPath(key, tags);
    if (path.isEmpty()) {
      return false;
    }
    QFileInfo fi(path);
    if (!fi.exists()) {
      return false;
    }
    if (fi.lastModified().secsTo(QDateTime::currentDateTime()) > MISS_TTL_SECONDS) {
      QFile::remove(path);
      return false;
    }
    return true;
  }

  void Store::storeMiss(const QString &key, const QStringList &tags) {
    const QString path = missPath(key, tags);
    if (path.isEmpty()) {
      return;
    }
    QSaveFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
      f.write(QByteArray()); // a sentinel; only its name and mtime matter
      f.commit();
    }
  }

  int Store::clear() {
    QDir dir(cache_dir);
    const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    int removed = 0;
    for (const auto &fi : entries) {
      if (QFile::remove(fi.absoluteFilePath())) {
        removed++;
      }
    }
    return removed;
  }

  void Store::trim(const QStringList &exts, int max_entries) const {
    QDir dir(cache_dir);
    QStringList masks;
    for (const auto &ext : exts) {
      masks << ("*." + ext);
    }
    const auto entries = dir.entryInfoList(masks, QDir::Files, QDir::Time);
    for (int i = max_entries; i < entries.size(); ++i) {
      QFile::remove(entries.at(i).absoluteFilePath());
    }
  }
}
