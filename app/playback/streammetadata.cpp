#include "streammetadata.h"

#include <QDebug>

namespace Playback {
  StreamMetaData::StreamMetaData() {
  }

  void StreamMetaData::insert(const QString &key, const QString &value) {
    _data.insert(key, value);
  }

  void StreamMetaData::clear() {
    _data.clear();
  }

  QMap<QString, QString> StreamMetaData::rawData() const {
    return _data;
  }

  quint16 StreamMetaData::bitrate() const {
    bool ok = false;
    auto i = _data.value("icy-br", "0").toUInt(&ok);
    if (!ok) {
      return 0;
    }
    return static_cast<quint16>(i);
  }
}
