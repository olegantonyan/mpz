#include "streammetadata.h"

#include <QDebug>

StreamMetaData::StreamMetaData() {
}

bool StreamMetaData::isEmpty() const {
  return _data.isEmpty();
}

void StreamMetaData::insert(const QString &key, const QString &value) {
  _data.insert(key, value);
    qDebug() << _data;
}

void StreamMetaData::clear() {
  _data.clear();
}

void StreamMetaData::setStream(const QString &str) {
  _data.insert("stream", str);
}

quint16 StreamMetaData::bitrate() const {
  bool ok = false;
  auto i = _data.value("icy-br", "0").toUInt(&ok);
  if (!ok) {
    return 0;
  }
  return static_cast<quint16>(i);
}

quint16 StreamMetaData::samplerate() const {
  bool ok = false;
  auto i = _data.value("icy-sr", "0").toUInt(&ok);
  if (!ok) {
    return 0;
  }
  return static_cast<quint16>(i);
}

QString StreamMetaData::stream() const {
  return _data.value("stream", "");
}
