#ifndef COVERSEMBEDDED_H
#define COVERSEMBEDDED_H

#include <QImage>
#include <QString>
#include <QHash>
#include <memory>
#include <QTemporaryFile>
#include <QByteArray>

namespace CoverArt {
  class Embedded {
  public:
    Embedded();
    ~Embedded();

    QString get(const QString &filepath);

  private:
    QHash <QString, std::shared_ptr<QTemporaryFile>> cache;
    QHash <QByteArray, std::shared_ptr<QTemporaryFile>> files_cache;
    QString temp_dir;

    QImage m4a(const QString &filepath) const;
    QImage mp3(const QString &filepath) const;
    QImage flac(const QString &filepath) const;

    std::shared_ptr<QTemporaryFile> save(const QImage &img);
    QByteArray image_to_bytearray(const QImage &image) const;
    QByteArray image_hash(const QByteArray &ba) const;
    std::shared_ptr<QTemporaryFile> create_tempfile();

  };
}
#endif // COVERSEMBEDDED_H
