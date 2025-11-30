#include "coverart/mpd.h"

#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QCryptographicHash>

static inline uint32_t read_u32_be_ptr(const char *p) {
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
    // RIFF....WEBP
    return b.size() >= 12 && memcmp(b.constData(), "RIFF", 4) == 0 &&
           memcmp(b.constData() + 8, "WEBP", 4) == 0;
}

static QString inferMimeFromMagic(const QByteArray &b) {
    if (looksLikeJPEG(b)) return QStringLiteral("image/jpeg");
    if (looksLikePNG(b))  return QStringLiteral("image/png");
    if (looksLikeWebP(b)) return QStringLiteral("image/webp");
    return QStringLiteral("application/octet-stream");
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

    if (src.isEmpty()) {
      return pictures;
    }

    const char *p = src.constData();
    const char *end = p + src.size();

    auto read_string = [&](const char *&ptr, QString &out) -> bool {
      if (ptr + 4 > end) return false;
      uint32_t len = read_u32_be_ptr(ptr);
      ptr += 4;
      if (len > static_cast<uint32_t>(end - ptr)) return false;
      out = QString::fromUtf8(ptr, int(len));
      ptr += len;
      return true;
    };

    // Try to parse at least one FLAC METADATA_BLOCK_PICTURE.
    const char *cursor = p;
    bool parsedAny = false;

    while (cursor + 4 <= end) {
      // Safely check we can read 'type'
      if (cursor + 4 > end) break;
      uint32_t type = read_u32_be_ptr(cursor);
      const char *after_type = cursor + 4;

      // Quick sanity: after_type must still allow reading two lengths (mime+desc) + 4*4 dims + data_len
      if (after_type + 4 > end) break; // not enough for mime length
      uint32_t mime_len = read_u32_be_ptr(after_type);
      // ensure mime length field + mime string exists
      if (after_type + 4 + mime_len > end) break;

      // Try to fully parse one block; if any bound check fails, abort FLAC parse attempt.
      const char *ptr = cursor;
      Picture pic;
      pic.type = read_u32_be_ptr(ptr); ptr += 4;

      if (!read_string(ptr, pic.mime)) { break; }
      if (!read_string(ptr, pic.description)) { break; }

      // need 4*4 bytes for width,height,depth,colors
      if (ptr + 16 > end) break;
      pic.width  = read_u32_be_ptr(ptr); ptr += 4;
      pic.height = read_u32_be_ptr(ptr); ptr += 4;
      pic.depth  = read_u32_be_ptr(ptr); ptr += 4;
      pic.colors = read_u32_be_ptr(ptr); ptr += 4;

      if (ptr + 4 > end) break;
      uint32_t data_len = read_u32_be_ptr(ptr); ptr += 4;

      if (data_len > static_cast<uint32_t>(end - ptr)) break;

      pic.data = QByteArray(ptr, int(data_len));
      ptr += data_len;

      // success: append and continue from ptr
      pictures.append(pic);
      parsedAny = true;
      cursor = ptr;
    }

    if (parsedAny) {
      // Parsed one or more FLAC picture blocks successfully
      return pictures;
    }

    // Fallback: maybe MPD returned raw image bytes (no FLAC header).
    // Detect JPEG/PNG/WebP magic and wrap as a single Picture.
    if (looksLikeJPEG(src) || looksLikePNG(src) || looksLikeWebP(src)) {
      Picture pic;
      pic.type = 3; // assume front cover if unknown
      pic.mime = inferMimeFromMagic(src);
      pic.description = QStringLiteral("external-image-or-raw");
      pic.width = pic.height = pic.depth = pic.colors = 0;
      pic.data = src;
      pictures.append(pic);
      return pictures;
    }

    // Nothing parsed and not a recognized raw image -> give debug info
    // print up to first 32 bytes hex for debugging
    const int dumpN = qMin<int>(32, src.size());
    QString hex;
    for (int i = 0; i < dumpN; ++i) {
        hex += QString::asprintf("%02X ", (unsigned char)src[i]);
    }
    return pictures;
  }

}
