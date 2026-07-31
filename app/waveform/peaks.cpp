#include "waveform/peaks.h"

#include <QDataStream>
#include <QIODevice>

#include <cstring>

namespace Waveform {
  namespace {
    const char MAGIC[4] = {'M', 'P', 'Z', 'W'};
    constexpr quint8 FORMAT = 1;
  }

  bool Peaks::isEmpty() const {
    return peak.isEmpty() || peak.size() != rms.size() || bucket_ms == 0;
  }

  QByteArray Peaks::serialize() const {
    if (isEmpty()) {
      return QByteArray();
    }

    QByteArray blob;
    QDataStream out(&blob, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    out.writeRawData(MAGIC, sizeof(MAGIC));
    out << FORMAT << bucket_ms << duration_ms << static_cast<quint32>(peak.size());
    out.writeRawData(reinterpret_cast<const char *>(peak.constData()), peak.size());
    out.writeRawData(reinterpret_cast<const char *>(rms.constData()), rms.size());

    return blob;
  }

  Peaks Peaks::deserialize(const QByteArray &blob) {
    QDataStream in(blob);
    in.setByteOrder(QDataStream::LittleEndian);

    char magic[sizeof(MAGIC)] = {};
    if (in.readRawData(magic, sizeof(magic)) != sizeof(magic) || memcmp(magic, MAGIC, sizeof(MAGIC)) != 0) {
      return Peaks();
    }

    quint8 version = 0;
    in >> version;
    if (version != FORMAT) {
      return Peaks();
    }

    Peaks p;
    quint32 count = 0;
    in >> p.bucket_ms >> p.duration_ms >> count;
    if (in.status() != QDataStream::Ok || count == 0 || p.bucket_ms == 0) {
      return Peaks();
    }
    if (blob.size() - in.device()->pos() != qint64(count) * 2) {
      return Peaks();
    }

    p.peak.resize(count);
    p.rms.resize(count);
    in.readRawData(reinterpret_cast<char *>(p.peak.data()), count);
    in.readRawData(reinterpret_cast<char *>(p.rms.data()), count);

    return p;
  }
}
