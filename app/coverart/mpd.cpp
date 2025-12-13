#include "coverart/mpd.h"

#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QCryptographicHash>

static inline uint32_t read_u32_be(const char *p) {
  return (uint32_t(uint8_t(p[0])) << 24) |
         (uint32_t(uint8_t(p[1])) << 16) |
         (uint32_t(uint8_t(p[2])) <<  8) |
         (uint32_t(uint8_t(p[3])));
}

static bool looksLikeJPEG(const QByteArray &b) {
  return b.size() >= 3 &&
         (uint8_t)b[0] == 0xFF &&
         (uint8_t)b[1] == 0xD8 &&
         (uint8_t)b[2] == 0xFF;
}

static bool looksLikePNG(const QByteArray &b) {
  static const char pngSig[] = "\x89""PNG\r\n\x1A\n";
  return b.size() >= 8 && memcmp(b.constData(), pngSig, 8) == 0;
}

static bool looksLikeWebP(const QByteArray &b) {
  return b.size() >= 12 && memcmp(b.constData(), "RIFF", 4) == 0 &&
         memcmp(b.constData() + 8, "WEBP", 4) == 0;
}

static QString inferMimeFromMagic(const QByteArray &b) {
  if (looksLikeJPEG(b)) return QStringLiteral("image/jpeg");
  if (looksLikePNG(b))  return QStringLiteral("image/png");
  if (looksLikeWebP(b)) return QStringLiteral("image/webp");
  return QStringLiteral("application/octet-stream");
}

static bool readString(const char *&ptr, const char *end, QString &out) {
    if (ptr + 4 > end) return false;
    uint32_t len = read_u32_be(ptr);
    ptr += 4;
    if (len > static_cast<uint32_t>(end - ptr)) return false;
    out = QString::fromUtf8(ptr, int(len));
    ptr += len;
    return true;
}

namespace CoverArt {
  Mpd::Mpd(MpdClient::Client &cl) : client(cl) {
    temp_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + "mpd_covers" + QDir::separator();
    if (!QDir(temp_dir).exists()) {
      QDir().mkdir(temp_dir);
    }
  }

  QString Mpd::get(const QString &filepath) {
    auto binary = fetch(filepath);
    auto hashed = image_hash(binary);

    if (binary.isEmpty()) {
      files_cache.insert(hashed, nullptr);
    }

    if (files_cache.contains(hashed)) {
      if (files_cache.value(hashed) == nullptr) {
        return "";
      } else {
        return files_cache.value(hashed)->fileName();
      }
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

  QByteArray Mpd::fetch(const QString &filepath) {
    QByteArray result = client.albumArt(filepath);
    if (result.isEmpty()) {
      auto pics = parsePictures(client.readPicture(filepath));
      if (!pics.isEmpty()) {
        for (auto it : pics) {
          if (it.isFrontCover()) {
            result = it.data;
            break;
          }
        }
        if (result.isEmpty()) {
          result = pics.first().data;
        }
      }
    }
    return result;
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

  QVector<Picture> Mpd::parsePictures(const QByteArray &src) {
    QVector<Picture> pictures;
    if (src.isEmpty()) return pictures;

    const char *p = src.constData();
    const char *end = p + src.size();

    const char *cursor = p;
    bool parsedAny = false;

    while (cursor + 4 <= end) {
      const char *ptr = cursor;

      // Check we can read type
      if (ptr + 4 > end) break;

      // Attempt to parse FLAC METADATA_BLOCK_PICTURE
      Picture pic;
      pic.type = read_u32_be(ptr); ptr += 4;

      if (!readString(ptr, end, pic.mime)) break;
      if (!readString(ptr, end, pic.description)) break;

      if (ptr + 16 > end) break;
      pic.width  = read_u32_be(ptr); ptr += 4;
      pic.height = read_u32_be(ptr); ptr += 4;
      pic.depth  = read_u32_be(ptr); ptr += 4;
      pic.colors = read_u32_be(ptr); ptr += 4;

      if (ptr + 4 > end) break;
      uint32_t data_len = read_u32_be(ptr); ptr += 4;

      if (data_len > static_cast<uint32_t>(end - ptr)) break;
      pic.data = QByteArray(ptr, int(data_len));
      ptr += data_len;

      pictures.append(pic);
      parsedAny = true;
      cursor = ptr;
    }

    if (parsedAny) return pictures;

    // Fallback: raw image bytes (JPEG/PNG/WebP)
    if (looksLikeJPEG(src) || looksLikePNG(src) || looksLikeWebP(src)) {
      Picture pic;
      pic.type = 3; // assume front cover
      pic.mime = inferMimeFromMagic(src);
      pic.description = QStringLiteral("external-image-or-raw");
      pic.data = src;
      pictures.append(pic);
    }

    return pictures;
  }

}
