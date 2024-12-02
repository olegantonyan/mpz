#include "coverart/embedded.h"

#ifdef USE_SYSTEM_TAGLIB
  #include "taglib/tag.h"
  #include "taglib/mp4file.h"
  #include "taglib/id3v2.h"
  #include "taglib/id3v2frame.h"
  #include "taglib/attachedpictureframe.h"
  #include "taglib/id3v2tag.h"
  #include "taglib/mpegfile.h"
  #include "taglib/flacfile.h"
  #include "taglib/flacpicture.h"
  #include "taglib/fileref.h"
  #include "taglib/tag.h"
  #include "taglib/tpropertymap.h"
#else
  #include "tag.h"
  #include "mp4/mp4file.h"
  #include "mpeg/id3v2/id3v2.h"
  #include "mpeg/id3v2/id3v2frame.h"
  #include "mpeg/id3v2/frames/attachedpictureframe.h"
  #include "mpeg/id3v2/id3v2tag.h"
  #include "mpeg/mpegfile.h"
  #include "flac/flacfile.h"
  #include"flac/flacpicture.h"
  #include "fileref.h"
  #include "tag.h"
  #include "tpropertymap.h"
#endif

#include <QDebug>
#include <QBuffer>
#include <QDir>
#include <QApplication>
#include <QCryptographicHash>

namespace CoverArt {
  Embedded::Embedded() {
    temp_dir = QDir::tempPath() + QDir::separator() + qAppName() + "_embedded_covers" + QDir::separator();
    if (!QDir(temp_dir).exists()) {
      QDir().mkdir(temp_dir);
    }
  }

  Embedded::~Embedded() {
    QDir(temp_dir).removeRecursively();
  }

  QString Embedded::get(const QString &filepath) {
    QString result;
    if (filepath.isEmpty()) {
      return result;
    }

    if (cache.contains(filepath)) {
      return cache.value(filepath)->fileName();
    }

    QImage img;
    if (filepath.endsWith("m4a", Qt::CaseInsensitive)) {
      img = m4a(filepath);
    } else if (filepath.endsWith("mp3", Qt::CaseInsensitive)) {
      img = mp3(filepath);
    } else if (filepath.endsWith("flac", Qt::CaseInsensitive)) {
      img = flac(filepath);
    }

    auto f = save(img);
    if (f != nullptr) {
      result = f->fileName();
      cache.insert(filepath, f);
      qDebug() << "embedded cover in" << f->fileName();
    }

    return result;
  }

  QImage Embedded::m4a(const QString &filepath) const {
    QImage image;

    TagLib::MP4::File f(filepath.toUtf8().constData());
    TagLib::MP4::Tag* tag = f.tag();
    if (tag != nullptr) {
      TagLib::MP4::ItemMap itemsListMap = tag->itemMap();
      TagLib::MP4::Item coverItem = itemsListMap["covr"];
      TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
      if (!coverArtList.isEmpty()) {
        TagLib::MP4::CoverArt coverArt = coverArtList.front();
        image.loadFromData((const unsigned char *)coverArt.data().data(), coverArt.data().size());
      }
    }
    return image;
  }

  QImage Embedded::mp3(const QString &filepath) const {
    QImage image;

    TagLib::MPEG::File f(filepath.toUtf8().constData());
    TagLib::ID3v2::Tag *tag = f.ID3v2Tag();
    if (tag != nullptr) {
      TagLib::ID3v2::FrameList l = tag->frameList("APIC");
      if (!l.isEmpty()) {
        for(TagLib::ID3v2::FrameList::ConstIterator it = l.begin(); it != l.end(); ++it) {
          auto picframe = (TagLib::ID3v2::AttachedPictureFrame *)(*it);
          if (picframe->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
            image.loadFromData((const unsigned char *)picframe->picture().data(), picframe->picture().size());
          }
        }
      }
    }

    return image;
  }

  QImage Embedded::flac(const QString &filepath) const {
    QImage image;

    TagLib::FLAC::File f(filepath.toUtf8().constData());
    auto pics = f.pictureList();
    if (!pics.isEmpty()) {
      auto pic = pics[0];
      image.loadFromData((const unsigned char *)pic->data().data(), pic->data().size());
    }

    return image;
  }

  std::shared_ptr<QTemporaryFile> Embedded::save(const QImage &img) {
    if (img.isNull()) {
      return nullptr;
    }

    auto ba = image_to_bytearray(img);
    auto hashed = image_hash(ba);
    if (files_cache.contains(hashed)) {
      return files_cache.value(hashed);
    }

    auto tmpfile = create_tempfile();
    if (!tmpfile->open()) {
      qDebug() << "cannot open tempfile" << tmpfile->fileName() << tmpfile->errorString();
      return nullptr;
    }
    tmpfile->setTextModeEnabled(false);
    if (tmpfile->write(ba) < 0) {
      qDebug() << "cannot write embedded cover to a tempfile" << tmpfile->fileName() << tmpfile->errorString();
      return nullptr;
    }
    tmpfile->close();

    files_cache.insert(hashed, tmpfile);

    return tmpfile;
  }

  QByteArray Embedded::image_to_bytearray(const QImage &image) const {
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return ba;
  }

  QByteArray Embedded::image_hash(const QByteArray &ba) const {
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(ba);
    return hash.result();
  }

  std::shared_ptr<QTemporaryFile> Embedded::create_tempfile() {
    auto name_template = temp_dir + "XXXXXX.png";
    return std::shared_ptr<QTemporaryFile>(new QTemporaryFile(name_template, qApp));
  }
}
