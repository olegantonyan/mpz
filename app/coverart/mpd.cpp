#include "coverart/mpd.h"

#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QCryptographicHash>

namespace CoverArt {
  Mpd::Mpd(MpdClient::Client &cl) : client(cl) {
    temp_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + "mpd_covers" + QDir::separator();
    if (!QDir(temp_dir).exists()) {
      QDir().mkdir(temp_dir);
    }
  }

  QString Mpd::get(const QString &filepath) {
    auto binary = client.albumArt(filepath);
    if (binary.isEmpty()) {
      return "";
    }

    auto hashed = image_hash(binary);
    if (files_cache.contains(hashed)) {
      return files_cache.value(hashed)->fileName();
    }

    auto tmpfile = create_tempfile();
    if (!tmpfile->open()) {
      qDebug() << "cannot open tempfile" << tmpfile->fileName() << tmpfile->errorString();
      return "";
    }
    tmpfile->setTextModeEnabled(false);
    if (tmpfile->write(binary) < 0) {
      qDebug() << "cannot write embedded cover to a tempfile" << tmpfile->fileName() << tmpfile->errorString();
      return "";
    }
    tmpfile->close();

    files_cache.insert(hashed, tmpfile);

    return tmpfile->fileName();
  }

  std::shared_ptr<QTemporaryFile> Mpd::create_tempfile() {
    auto name_template = temp_dir + "XXXXXX.png";
    return std::shared_ptr<QTemporaryFile>(new QTemporaryFile(name_template, qApp));
  }

  QByteArray Mpd::image_hash(const QByteArray &ba) const {
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(ba);
    return hash.result();
  }
}
